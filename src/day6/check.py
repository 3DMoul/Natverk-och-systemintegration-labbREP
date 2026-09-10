"""Run fresh TCP connections from named lab namespaces."""
import subprocess
import sys

probe = '''
import socket
import sys

port = int(sys.argv[1])
try:
    with socket.create_connection(("10.60.20.10", port), timeout=1) as connection:
        connection.settimeout(1)
        data = connection.recv(100)
        if data == b"IOT25 TCP probe OK\\n":
            sys.exit(0)
        sys.exit(2)
except OSError:
    sys.exit(1)
'''
broken = len(sys.argv) > 1 and sys.argv[1] == 'broken'
existing = subprocess.check_output(['ip', 'netns', 'list'], text=True)
present = {line.split()[0] for line in existing.splitlines() if line.strip()}
required = {'iot25-d6-' + name for name in ('srv', 'iot', 'admin', 'router')}
if not required.issubset(present):
    raise SystemExit('Lab namespaces are missing; keep the WSL terminal open and run up again')
cases = [
    ('srv', 1883, True),
    ('srv', 8080, True),
    ('iot', 1883, not broken),
    ('iot', 8080, False),
    ('admin', 8080, not broken),
    ('admin', 1883, False),
]

failed = 0
for source, port, expected in cases:
    command = [
        'ip', 'netns', 'exec', 'iot25-d6-' + source,
        'python3', '-c', probe, str(port),
    ]
    result = subprocess.run(command, timeout=4)
    actual = result.returncode == 0
    ok = result.returncode in (0, 1) and actual == expected
    if not ok:
        failed += 1

    print(
        'PASS' if ok else 'FAIL',
        source, '->', port,
        'ALLOW' if actual else 'BLOCK',
        'expected', 'ALLOW' if expected else 'BLOCK',
    )

raise SystemExit(1 if failed else 0)
