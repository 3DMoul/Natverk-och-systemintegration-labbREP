"""Real loopback MQTT + C++ HTTP integration, no external services."""
import json
import pathlib
import socket
import subprocess
import sys
import tempfile
import time
import urllib.request
import urllib.error

root = pathlib.Path(__file__).resolve().parent
binary = pathlib.Path(sys.argv[1]).resolve()
mosq = pathlib.Path(sys.argv[2]).resolve()
suffix = '.exe' if sys.platform == 'win32' else ''


def exe(name):
    return str(binary / (name + suffix))


def mqtt(name):
    return str(mosq / (name + suffix))


def wait_port(port):
    for _ in range(60):
        try:
            with socket.create_connection(('127.0.0.1', port), timeout=0.1):
                return
        except OSError:
            time.sleep(0.1)
    raise RuntimeError('Server did not start: ' + str(port))


def request(path, body=None, content_type='application/json'):
    req = urllib.request.Request(
        'http://127.0.0.1:8085' + path,
        data=body,
        headers={'Content-Type': content_type},
    )
    try:
        with urllib.request.urlopen(req, timeout=3) as response:
            return response.status, response.read()
    except urllib.error.HTTPError as error:
        return error.code, error.read()


def run(name, *args, expected=0):
    result = subprocess.run(
        [exe(name), *map(str, args)],
        capture_output=True,
        text=True,
        timeout=8,
    )
    assert result.returncode == expected, (
        name, result.returncode, result.stdout, result.stderr
    )
    return result.stdout


processes = []
try:
    # Refuse to test against or interfere with an existing local service.
    for port in (1885, 8085):
        with socket.socket() as listener:
            listener.bind(('127.0.0.1', port))

    broker = subprocess.Popen(
        [mqtt('mosquitto'), '-c', str(root / 'mosquitto.conf')],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    processes.append(broker)

    api = subprocess.Popen(
        [exe('api')],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    processes.append(api)
    wait_port(1885)
    wait_port(8085)
    assert request('/api/readings/latest')[0] == 404

    with tempfile.TemporaryDirectory(prefix='iot25-day5-') as tmp:
        reading = pathlib.Path(tmp) / 'reading.json'
        received = pathlib.Path(tmp) / 'received.json'
        run('sensor', reading)
        assert json.loads(reading.read_bytes()) == json.loads(
            (root / 'testdata/valid.json').read_bytes()
        )

        # Retained test data makes subscription readiness deterministic.
        publish_command = [
            mqtt('mosquitto_pub'),
            '-h', '127.0.0.1',
            '-p', '1885',
            '-t', 'iot25/dag5/reading',
            '-r',
            '-f', str(reading),
        ]
        subprocess.run(publish_command, check=True, timeout=5)

        capture_command = [
            sys.executable,
            str(root / 'capture.py'),
            mqtt('mosquitto_sub'),
            str(received),
        ]
        subprocess.run(capture_command, check=True, timeout=20)

        # A message on another topic must not reach this subscription.
        wrong_topic_command = [
            mqtt('mosquitto_pub'),
            '-h', '127.0.0.1',
            '-p', '1885',
            '-t', 'iot25/dag5/other',
            '-r',
            '-f', str(reading),
        ]
        subprocess.run(wrong_topic_command, check=True, timeout=5)
        subscription = subprocess.run(
            [
                mqtt('mosquitto_sub'),
                '-h', '127.0.0.1',
                '-p', '1885',
                '-t', 'iot25/dag5/reading',
                '-R',  # Ignore the earlier valid retained reading.
                '-C', '1',
                '-W', '1',
            ],
            capture_output=True,
            timeout=5,
        )
        assert subscription.returncode == 27, subscription.stderr
        assert subscription.stdout == b''
        print('PASS: wrong MQTT topic produces no matching payload')

        assert json.loads(reading.read_bytes()) == json.loads(received.read_bytes())
        assert 'HTTP 201' in run('bridge', received)
        assert '21.7 C' in run('consumer')
        baseline = request('/api/readings/latest')

        invalid_file = root / 'testdata/invalid.json'
        run('bridge', invalid_file, expected=1)
        assert request('/api/readings', invalid_file.read_bytes())[0] == 400
        assert request('/api/readings', b'{')[0] == 400
        assert request('/api/readings', reading.read_bytes(), 'text/plain')[0] == 415
        assert request('/api/readings', b'x' * 4097)[0] == 413
        assert request('/api/readings/latest') == baseline

        # Check the same files that the teacher uses in the demonstration.
        invalid_scenarios = {
            'invalid.json': 'expected sensorId:string',
            'invalid_syntax.json': 'parse_error',
            'missing_sensor_id.json': 'expected sensorId:string',
            'empty_sensor_id.json': 'sensorId must not be empty',
            'wrong_unit.json': 'unit must be C',
            'below_range.json': 'value must be finite and between -50 and 100',
            'above_range.json': 'value must be finite and between -50 and 100',
        }
        for filename, expected_error in invalid_scenarios.items():
            fixture = root / 'testdata' / filename
            run('bridge', fixture, expected=1)
            assert request('/api/readings/latest') == baseline

            status, response_body = request('/api/readings', fixture.read_bytes())
            assert status == 400, filename
            error = json.loads(response_body)['error']
            assert expected_error in error, (filename, error)
            assert request('/api/readings/latest') == baseline
            print('PASS:', filename, 'rejected without changing stored data')

        valid_scenarios = [
            'valid.json',
            'lower_boundary.json',
            'upper_boundary.json',
            'extra_field.json',
        ]
        for filename in valid_scenarios:
            fixture = root / 'testdata' / filename
            source = json.loads(fixture.read_bytes())
            expected_reading = {
                field: source[field] for field in ('sensorId', 'value', 'unit')
            }
            assert 'HTTP 201' in run('bridge', fixture)
            status, response_body = request('/api/readings/latest')
            assert status == 200, filename
            assert json.loads(response_body) == expected_reading, filename

            status, response_body = request('/api/readings', fixture.read_bytes())
            assert status == 201, filename
            assert json.loads(response_body) == expected_reading, filename
            print('PASS:', filename, 'accepted with expected fields and value')

        # Restore the ordinary demonstration value before the outage scenario.
        run('bridge', received)

        api.terminate()
        api.wait(timeout=5)
        run('bridge', received, expected=1)

        api = subprocess.Popen(
            [exe('api')],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        processes.append(api)
        wait_port(8085)
        assert request('/api/readings/latest')[0] == 404
        run('bridge', received)
        assert request('/api/readings/latest')[0] == 200
    print('PASS: MQTT, HTTP, validation, unchanged state, outage, restart, recovery')
finally:
    for process in reversed(processes):
        if process.poll() is None:
            process.terminate()
        process.wait(timeout=5)
