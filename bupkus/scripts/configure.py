import argparse
import pathlib

ROOT_DIR = pathlib.Path(__file__).resolve().parent.parent.parent
EXPORT_DIR = ROOT_DIR / "_export" / "bupkus"
BUPKUS_INSTALLER = "bupkus-installer.exe"

# stamped configuration values - found and replaced in the installer binary
BUPKUS_LISTENER_NAME_STAMP = "BASKETBALLJONES"
BUPKUS_HOSTNAME_STAMP = "EVERYBODYGETUP"
BUPKUS_AUTH_TOKEN_STAMP = "00000000-0000-0000-0000-000000000000"


def _stamp_value(binary: bytes, stamp_pattern: bytes, stamp_data: bytes) -> bytes:
    idx = binary.index(stamp_pattern)
    return binary.replace(
        binary[idx : idx + len(stamp_data) + 1],
        stamp_data + b"\x00",
        1,
    )


def configure_bupkus_installer(
    debug: bool, hostname: str, auth_token: str, listener_name: str
) -> bytes:
    installer_path = EXPORT_DIR / ("debug" if debug else "release") / BUPKUS_INSTALLER
    with open(installer_path, "rb") as f:
        installer_bin = f.read()
    installer_bin = _stamp_value(
        installer_bin, BUPKUS_HOSTNAME_STAMP.encode("utf-16-le"), hostname.encode("utf-16-le")
    )
    installer_bin = _stamp_value(
        installer_bin, BUPKUS_AUTH_TOKEN_STAMP.encode("utf-16-le"), auth_token.encode("utf-16-le")
    )
    installer_bin = _stamp_value(
        installer_bin, BUPKUS_LISTENER_NAME_STAMP.encode("utf-16-le"), listener_name.encode("utf-16-le")
    )
    return installer_bin


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--listener-name",
        default="bupkus",
        help="Name for the listner application (does not include file extension)",
    )
    parser.add_argument(
        "--hostname",
        required=True,
        help="HTTPS hostname",
    )
    parser.add_argument(
        "--auth-token",
        default="00000000-0000-0000-0000-000000000000",
        help="API token (UUID)",
    )
    parser.add_argument("-d", "--debug", action="store_true", help="Configure debug binaries")
    parser.add_argument(
        "-o",
        "--output",
        type=pathlib.Path,
        default=BUPKUS_INSTALLER,
        help="Configured installer output path",
    )
    args = parser.parse_args()
    output_bin = configure_bupkus_installer(
        debug=args.debug,
        hostname=args.hostname,
        auth_token=args.auth_token,
        listener_name=args.listener_name,
    )
    with args.output.open("wb") as f:
        f.write(output_bin)


if __name__ == "__main__":
    main()
