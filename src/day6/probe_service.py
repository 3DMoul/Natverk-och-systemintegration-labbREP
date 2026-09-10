"""TCP reachability targets, deliberately not MQTT or HTTP implementations."""
import socket
import threading


def serve(port):
    with socket.socket() as listener:
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind(('10.60.20.10', port))
        listener.listen(8)
        while True:
            client, _ = listener.accept()
            with client:
                client.settimeout(1)
                try:
                    client.sendall(b'IOT25 TCP probe OK\n')
                except OSError:
                    pass


workers = []
for port in (1883, 8080):
    worker = threading.Thread(target=serve, args=(port,))
    workers.append(worker)

for worker in workers:
    worker.start()

for worker in workers:
    worker.join()
