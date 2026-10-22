#!/usr/bin/env python3
"""Run MYB byte-exact black-box cases without sharing their working directories."""
from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
import math
from pathlib import Path, PurePosixPath, PureWindowsPath
import re
import shutil
import subprocess
import sys
import tempfile

DEFAULT_MANIFEST = Path(__file__).resolve().parents[1] / "tests/blackbox/manifest.json"


class ManifestError(ValueError):
    pass


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ManifestError(message)


def text(value: object, description: str) -> str:
    require(isinstance(value, str) and bool(value) and "\0" not in value,
            f"{description} must be a nonempty string without NUL")
    return value


def relative_path(value: object, description: str) -> Path:
    value = text(value, description)
    windows = PureWindowsPath(value)
    posix = PurePosixPath(value.replace("\\", "/"))
    require(not windows.drive and not windows.root and not posix.is_absolute()
            and ".." not in posix.parts, f"{description} must stay inside the isolated directory")
    require(all(":" not in component for component in posix.parts),
            f"{description} cannot name an alternate data stream")
    return Path(*posix.parts)


def fixture(base: Path, value: object, description: str) -> Path:
    value = text(value, description)
    require(not Path(value).is_absolute() and not PureWindowsPath(value).drive,
            f"{description} must be relative to the manifest")
    path = (base / value).resolve()
    require(path.is_file(), f"{description} does not exist: {path}")
    return path


@dataclass(frozen=True)
class Case:
    id: str
    script: Path
    mode: str
    stdin: bytes
    exit: int
    stdout: bytes
    stderr: bytes | None
    stderr_regex: str | None
    timeout: float
    cwd: Path
    files: tuple[tuple[Path, Path], ...]
    expected_files: tuple[tuple[Path, bytes | None], ...]


def load_manifest(path: Path) -> list[Case]:
    try:
        root = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError, UnicodeError) as error:
        raise ManifestError(f"cannot read manifest {path}: {error}") from error
    require(isinstance(root, dict), "manifest root must be an object")
    require(type(root.get("schema_version")) is int and root["schema_version"] == 1,
            "manifest schema_version must be 1")
    require(isinstance(root.get("cases"), list), "manifest cases must be a list")
    require(set(root) <= {"schema_version", "cases"}, "unknown manifest field")
    base = path.resolve().parent
    cases: list[Case] = []
    identifiers: set[str] = set()
    allowed = {"id", "script", "mode", "stdin_file", "expect_exit", "expect_stdout_file",
               "expect_stderr_file", "expect_stderr_regex", "timeout_seconds", "files", "expect_files", "cwd"}
    for raw in root["cases"]:
        require(isinstance(raw, dict), "each case must be an object")
        require(set(raw) <= allowed, "unknown case field")
        identifier = text(raw.get("id"), "case id")
        require(identifier not in identifiers, f"duplicate case id: {identifier}")
        identifiers.add(identifier)
        script = fixture(base, raw.get("script"), f"{identifier}: script")
        mode = raw.get("mode", "run")
        require(mode in ("run", "check"), f"{identifier}: mode must be run or check")
        exit_code = raw.get("expect_exit")
        require(type(exit_code) is int, f"{identifier}: expect_exit must be an integer")
        stdout = fixture(base, raw.get("expect_stdout_file"), f"{identifier}: stdout fixture").read_bytes()
        require(("expect_stderr_file" in raw) != ("expect_stderr_regex" in raw),
                f"{identifier}: specify exactly one stderr expectation")
        stderr = None
        pattern = None
        if "expect_stderr_file" in raw:
            stderr = fixture(base, raw["expect_stderr_file"], f"{identifier}: stderr fixture").read_bytes()
        else:
            pattern = raw["expect_stderr_regex"]
            require(isinstance(pattern, str), f"{identifier}: stderr regex must be a string")
            try:
                re.compile(pattern.replace("$SCRIPT", "script"))
            except re.error as error:
                raise ManifestError(f"{identifier}: invalid stderr regex: {error}") from error
        timeout = raw.get("timeout_seconds", 5)
        require(type(timeout) in (int, float) and math.isfinite(timeout) and timeout > 0,
                f"{identifier}: timeout_seconds must be positive and finite")
        stdin = fixture(base, raw["stdin_file"], f"{identifier}: stdin fixture").read_bytes() if "stdin_file" in raw else b""
        cwd = relative_path(raw.get("cwd", "."), f"{identifier}: cwd")
        files: list[tuple[Path, Path]] = []
        require(isinstance(raw.get("files", []), list), f"{identifier}: files must be a list")
        for item in raw.get("files", []):
            require(isinstance(item, dict) and set(item) == {"from", "to"},
                    f"{identifier}: files entry needs from and to")
            source = fixture(base, item["from"], f"{identifier}: file fixture")
            target = relative_path(item["to"], f"{identifier}: file target")
            require(target != Path(script.name), f"{identifier}: file target replaces entry script")
            files.append((source, target))
        expected_files: list[tuple[Path, bytes | None]] = []
        require(isinstance(raw.get("expect_files", []), list), f"{identifier}: expect_files must be a list")
        for item in raw.get("expect_files", []):
            require(isinstance(item, dict), f"{identifier}: expect_files entry must be an object")
            require(set(item) in ({"path", "content_file"}, {"path", "absent"}),
                    f"{identifier}: file expectation needs content_file or absent=true")
            target = relative_path(item["path"], f"{identifier}: expected file path")
            if "absent" in item:
                require(item["absent"] is True, f"{identifier}: absent must be true")
                expected_files.append((target, None))
            else:
                expected_files.append((target, fixture(base, item["content_file"],
                                                       f"{identifier}: expected file fixture").read_bytes()))
        cases.append(Case(identifier, script, mode, stdin, exit_code, stdout, stderr, pattern,
                          float(timeout), cwd, tuple(files), tuple(expected_files)))
    return cases


def byte_difference(label: str, expected: bytes, actual: bytes) -> str:
    return f"{label}: expected {expected[:160]!r} ({len(expected)} bytes), actual {actual[:160]!r} ({len(actual)} bytes)"


def run_case(interpreter: Path, case: Case) -> list[str]:
    errors: list[str] = []
    with tempfile.TemporaryDirectory(prefix="myb-blackbox-") as temporary:
        directory = Path(temporary).resolve()
        script = directory / case.script.name
        shutil.copyfile(case.script, script)
        for source, target in case.files:
            destination = directory / target
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(source, destination)
        cwd = directory / case.cwd
        cwd.mkdir(parents=True, exist_ok=True)
        command = [str(interpreter)]
        if case.mode == "check":
            command.append("--check")
        command.append(str(script))
        try:
            child = subprocess.run(command, input=case.stdin, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, cwd=cwd, timeout=case.timeout,
                                   shell=False)
        except subprocess.TimeoutExpired as timeout:
            return [f"timed out after {case.timeout:g}s; no exit code; stdout={timeout.stdout!r}; stderr={timeout.stderr!r}"]
        except OSError as error:
            return [f"could not start interpreter: {error}"]
        if child.returncode != case.exit:
            errors.append(f"expected exit {case.exit}, actual exit {child.returncode}")
        if child.stdout != case.stdout:
            errors.append(byte_difference("stdout", case.stdout, child.stdout))
        if case.stderr is not None:
            if child.stderr != case.stderr:
                errors.append(byte_difference("stderr", case.stderr, child.stderr))
        else:
            try:
                decoded = child.stderr.decode("utf-8", errors="strict")
            except UnicodeDecodeError as error:
                errors.append(f"stderr is not valid UTF-8: {error}")
            else:
                pattern = case.stderr_regex.replace("$SCRIPT", re.escape(str(script)))
                if re.fullmatch(pattern, decoded) is None:
                    errors.append(f"stderr regex {pattern!r} does not fully match {decoded[:320]!r}")
        for target, expected in case.expected_files:
            destination = directory / target
            # Follow-up reads must not follow an interpreter-created link outside its workspace.
            if not destination.resolve().is_relative_to(directory):
                errors.append(f"file {target}: resolves outside the isolated directory")
            elif expected is None:
                if destination.exists() or destination.is_symlink():
                    errors.append(f"file {target}: expected absent")
            else:
                try:
                    actual = destination.read_bytes()
                except OSError as error:
                    errors.append(f"file {target}: cannot read: {error}")
                else:
                    if actual != expected:
                        errors.append(byte_difference(f"file {target}", expected, actual))
        if errors:
            errors.insert(0, f"actual exit {child.returncode}")
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--interpreter", required=True, type=Path)
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    arguments = parser.parse_args(argv)
    interpreter = arguments.interpreter.resolve()
    try:
        require(interpreter.is_file(), f"interpreter does not exist: {interpreter}")
        cases = load_manifest(arguments.manifest)
    except (ManifestError, OSError) as error:
        print(f"driver error: {error}", file=sys.stderr)
        return 2
    failures = 0
    for case in cases:
        try:
            errors = run_case(interpreter, case)
        except OSError as error:
            errors = [f"case setup or result I/O failed: {error}"]
        if errors:
            failures += 1
            print(f"FAIL {case.id}")
            for error in errors:
                print(f"  {error}")
        else:
            print(f"PASS {case.id}")
    print(f"{len(cases) - failures} passed, {failures} failed, {len(cases)} total")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
