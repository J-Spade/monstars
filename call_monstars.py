"""Everybody get up, it's time to slam now!"""
import argparse
import contextlib
import socket

MAGIC_SRC_PORT = 31337

SUPPORTED_CMDS = [
    "ping",
]

SUPPORTED_PROTOS = [
    "tcp",
    "udp",
]


@contextlib.contextmanager
def _client_socket(proto):
    """helper function to create the client socket"""
    sock_proto = socket.SOCK_STREAM if proto == "tcp" else socket.SOCK_DGRAM
    sock = socket.socket(socket.AF_INET, sock_proto)
    sock.bind(("0.0.0.0", MAGIC_SRC_PORT))
    sock.settimeout(0)
    yield sock
    sock.close()


def _send_magic(sock, ip, port, data):
    """helper function to send the magic cmd packet"""
    print(f"sending magic packet --> {ip}:{port}")
    if sock.type == socket.SOCK_DGRAM:
        sock.sendto(data, (ip, port))
    # TODO: support TCP payload data (probably needs SOCK_RAW)
    elif sock.type == socket.SOCK_STREAM:
        try:
            sock.connect((ip, port))
        except BlockingIOError:
            pass


def do_ping(args):
    """send a ping"""
    with _client_socket(args.proto) as s:
        _send_magic(s, args.ip, args.dest_port, b"PING")
    # TODO: expect response pong


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
        "-p", "--proto",
        choices=SUPPORTED_PROTOS,
        default="udp",
        help="transport-layer protocol to use",
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
