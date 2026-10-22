"""Exercise the real runner using an independent executable fixture."""
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
RUNNER = ROOT / "tools" / "run_blackbox.py"
CHILD = Path(__file__).with_name("child_fixture.py")


class BlackboxDriverTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="myb-driver-test-")
        self.addCleanup(self.temporary.cleanup)
        self.directory = Path(self.temporary.name)
        shutil.copyfile(CHILD, self.directory / "fixture child.py")
        (self.directory / "stdout.bin").write_bytes(b"a\r\nb\n")
        (self.directory / "stderr.bin").write_bytes(b"fixture error\n")
        (self.directory / "empty.bin").write_bytes(b"")

    def case(self, name="bytes", **changes):
        case = dict(id=name, script="fixture child.py", expect_exit=7,
                    expect_stdout_file="stdout.bin", expect_stderr_file="stderr.bin")
        case.update(changes)
        return case

    def run_cases(self, cases, **root_changes):
        manifest = dict(schema_version=1, cases=cases)
        manifest.update(root_changes)
        path = self.directory / "manifest.json"
        path.write_text(json.dumps(manifest), encoding="utf-8")
        return subprocess.run([sys.executable, str(RUNNER), "--interpreter", sys.executable,
                               "--manifest", str(path)], capture_output=True, timeout=15)

    def test_exact_binary_output_and_exit(self):
        result = self.run_cases([self.case()])
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn(b"1 passed", result.stdout)

    def test_bad_golden_fails_and_next_case_runs(self):
        result = self.run_cases([self.case("bad", expect_stdout_file="empty.bin"), self.case("next")])
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn(b"PASS next", result.stdout)
        self.assertIn(b"stdout", result.stdout)
        self.assertIn(b"actual exit 7", result.stdout)

    def test_timeout_is_reported_and_next_case_runs(self):
        (self.directory / "timeout.in").write_bytes(b"timeout")
        result = self.run_cases([self.case("slow", stdin_file="timeout.in", timeout_seconds=0.1),
                                 self.case("after-timeout")])
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn(b"timed out", result.stdout)
        self.assertIn(b"PASS after-timeout", result.stdout)

    def test_rejects_unsafe_side_effect_paths_before_running(self):
        for field, value in (("files", [{"from": "empty.bin", "to": "../escape"}]),
                             ("expect_files", [{"path": "../escape", "absent": True}]),
                             ("cwd", "../escape"),
                             ("cwd", "C:\\escape"),
                             ("cwd", "..\\escape")):
            with self.subTest(field=field, value=value):
                result = self.run_cases([self.case(**{field: value})])
                self.assertEqual(result.returncode, 2, result.stdout + result.stderr)
                self.assertNotIn(b"PASS", result.stdout)

    def test_strict_utf8_regex_does_not_replace_invalid_bytes(self):
        (self.directory / "invalid.in").write_bytes(b"invalid-utf8")
        case = self.case(stdin_file="invalid.in", expect_exit=0,
                         expect_stdout_file="empty.bin", expect_stderr_regex=".*")
        del case["expect_stderr_file"]
        result = self.run_cases([case])
        self.assertEqual(result.returncode, 1, result.stdout + result.stderr)
        self.assertIn(b"UTF-8", result.stdout)

    def test_regex_script_placeholder_is_literal_and_full_match(self):
        (self.directory / "path.in").write_bytes(b"script-path")
        case = self.case(stdin_file="path.in", expect_exit=0,
                         expect_stdout_file="empty.bin", expect_stderr_regex="$SCRIPT")
        del case["expect_stderr_file"]
        result = self.run_cases([case])
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        case["expect_stderr_regex"] = "fixture"
        result = self.run_cases([case])
        self.assertEqual(result.returncode, 1)

    def test_files_are_staged_and_checked_inside_isolated_cwd(self):
        (self.directory / "files.in").write_bytes(b"files")
        payload = b"\x00\xff\r\n"
        (self.directory / "payload.bin").write_bytes(payload)
        case = self.case(stdin_file="files.in", expect_exit=0,
                         expect_stdout_file="payload.bin", expect_stderr_file="empty.bin",
                         cwd="nested workspace", files=[{"from": "payload.bin", "to": "nested workspace/input.bin"}],
                         expect_files=[{"path": "nested workspace/output.bin", "content_file": "payload.bin"},
                                       {"path": "must-not-exist", "absent": True}])
        result = self.run_cases([case])
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertFalse((self.directory / "nested workspace").exists())

    def test_rejects_bad_manifest_and_duplicate_ids(self):
        for cases, extra in (([self.case(), self.case()], {}), ([self.case()], {"schema_version": 2}),
                             ([self.case(timeout_seconds=0)], {}),
                             ([self.case(expect_stderr_regex="[")], {})):
            result = self.run_cases(cases, **extra)
            self.assertEqual(result.returncode, 2, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
