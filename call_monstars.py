"""Everybody get up, it's time to slam now!"""
import argparse
import base64
import contextlib
import socket

MAGIC_SRC_PORT = 31337
SOCKET_TIMEOUT = 10
RESPONSE_SIZE = 4196

SUPPORTED_CMDS = [
    "ping",
]

@contextlib.contextmanager
def _tcp_listener():
    """socket listening for the command response"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(("0.0.0.0", MAGIC_SRC_PORT))
    sock.settimeout(SOCKET_TIMEOUT)
    sock.listen(1)
    yield sock
    sock.close()


@contextlib.contextmanager
def _udp_sender():
    """socket for sending the command"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", MAGIC_SRC_PORT))
    yield sock
    sock.close()


def _send_cmd(ip, port, msg):
    """sends a magic cmd packet and returns the response"""
    encoded = base64.b64encode(msg.encode("ascii")) + b"\x00"

    with _tcp_listener() as tcp:
        with _udp_sender() as udp:
            print(f"sending magic packet --> {ip}:{port}")
            udp.sendto(encoded, (ip, port))
        print("waiting for reply...")
        try:
            sock, addr = tcp.accept()
        except socket.timeout as err:
            raise ConnectionError("No response received!") from err
        response = b""
        while data := sock.recv(4096):
            response += data
    return base64.b64decode(response).decode("ascii")


def do_ping(args):
    """send a ping"""
    msg = "PING"
    response = _send_cmd(args.ip, args.dest_port, msg)
    if "PONG" not in response:
        raise ValueError("Invalid response received!")
    print(f"Received PONG from {args.ip}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="send it!")
    # subparsers = parser.add_subparsers(help="cmd params")
    parser.add_argument(
        "command",
        default="ping",
        choices=SUPPORTED_CMDS,
        help="command to send",
    )
    parser.add_argument(
        "-i", "--ip",
        required=True,
        help="IP address of host machine",
    )
    parser.add_argument(
        "-d", "--dest-port",
        type=int,
        default=1234,
        help="destination port number",
    )
    args = parser.parse_args()
    cmd = globals().get(f"do_{args.command}")
    cmd(args)
