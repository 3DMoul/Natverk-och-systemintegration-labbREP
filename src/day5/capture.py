"""Capture exactly one MQTT payload without shell encoding conversions."""
import pathlib
import subprocess
import sys

if len(sys.argv) != 3:
    raise SystemExit('Usage: python capture.py PATH_TO_MOSQUITTO_SUB OUTPUT.json')
target = pathlib.Path(sys.argv[2])
# Do not leave a previous successful reading behind after a failed capture.
target.write_bytes(b'')
command = [
    sys.argv[1],
    '-h', '127.0.0.1',
    '-p', '1885',
    '-t', 'iot25/dag5/reading',
    '-C', '1',
    '-W', '15',
    '-F', '%p',
    '-N',
]
result = subprocess.run(command, capture_output=True, timeout=20)

if result.returncode:
    sys.stderr.buffer.write(result.stderr)
    raise SystemExit(result.returncode)
if not result.stdout:
    raise SystemExit('No payload received')
target.write_bytes(result.stdout)
print('Received', len(result.stdout), 'bytes in', target)
