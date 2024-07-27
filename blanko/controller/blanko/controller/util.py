import contextlib
import socket

SOCKET_TIMEOUT = 15
MAGIC_SRC_PORT = 31337

@contextlib.contextmanager
def tcp_listener(port, queue_size=1):
    """socket listening for the command response"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(("0.0.0.0", port))
    sock.settimeout(SOCKET_TIMEOUT)
    sock.listen(queue_size)
    try:
        yield sock
    finally:
        sock.close()


@contextlib.contextmanager
def udp_sender(src_port):
    """socket for sending the command"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", src_port))
    try:
        yield sock
    finally:
        sock.close()
