import argparse
import pathlib
import uuid

EXPORT_DIR = pathlib.Path(__file__).resolve().parent.parent / "export"
DEBUG_DIR = EXPORT_DIR / "Debug"
RELEASE_DIR = EXPORT_DIR / "Release"
BANG_INSTALLER_NAME = "installer.exe"

# stamped configuration values - found and replaced in the installer binary
BANG_SSP_NAME_STAMP = "BASKETBALLJONES".encode("utf-16-le")
BANG_HOSTNAME_STAMP = "EVERYBODYGETUP".encode("utf-16-le")
BANG_AUTH_TOKEN_STAMP = "00000000-0000-0000-0000-000000000000".encode("utf-16-le")


def _stamp_value(binary: bytes, stamp_pattern: bytes, stamp_data: bytes) -> bytes:
    idx = binary.index(stamp_pattern)
    return binary.replace(binary[idx : idx + len(stamp_data) + 1], stamp_data + b"\x00")


def configure_bang_installer(debug: bool, hostname: str, auth_token: str, provider_name: str) -> bytes:
    installer_path = (DEBUG_DIR if debug else RELEASE_DIR) / BANG_INSTALLER_NAME
    with open(installer_path, "rb") as f:
        installer_bin = f.read()

    installer_bin = _stamp_value(installer_bin, BANG_HOSTNAME_STAMP, hostname.encode("utf-16-le"))
    installer_bin = _stamp_value(installer_bin, BANG_AUTH_TOKEN_STAMP, auth_token.encode("utf-16-le"))
    installer_bin = _stamp_value(installer_bin, BANG_SSP_NAME_STAMP, provider_name.encode("utf-16-le"))

    return installer_bin


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--ssp-name", default="bang", help="Name for the SSP (does not include .dll)")
    parser.add_argument("--hostname", default="172.17.224.1", help="HTTPS hostname")
    parser.add_argument(
        "--auth-token", default="00000000-0000-0000-0000-000000000000", help="API token (UUID)"
    )
    parser.add_argument("-d", "--debug", action="store_true", help="Configure debug binaries")
    parser.add_argument(
        "-o", "--output", default="installer.exe", type=pathlib.Path, help="Configured installer output path"
    )
    args = parser.parse_args()

    output_bin = configure_bang_installer(
        debug=args.debug,
        hostname=args.hostname,
        auth_token=args.auth_token,
        provider_name=args.ssp_name,
    )
    with args.output.open("wb") as f:
        f.write(output_bin)


if __name__ == "__main__":
    main()
