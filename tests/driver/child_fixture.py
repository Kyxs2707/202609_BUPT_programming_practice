"""Independent child process used to verify the black-box runner."""
import pathlib
import sys
import time

mode = sys.stdin.buffer.read().decode("ascii")
if mode == "timeout":
    time.sleep(30)
elif mode == "invalid-utf8":
    sys.stderr.buffer.write(b"\xff")
elif mode == "script-path":
    sys.stderr.buffer.write(str(pathlib.Path(__file__).resolve()).encode("utf-8"))
elif mode == "files":
    payload = pathlib.Path("input.bin").read_bytes()
    pathlib.Path("output.bin").write_bytes(payload)
    sys.stdout.buffer.write(payload)
else:
    sys.stdout.buffer.write(b"a\r\nb\n")
    sys.stderr.buffer.write(b"fixture error\n")
    sys.exit(7)
