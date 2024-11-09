import argparse
import pathlib
import uuid

ROOT_DIR = pathlib.Path(__file__).resolve().parent.parent.parent
EXPORT_DIR = ROOT_DIR / "_export" / "bang"

BANG_INSTALLER_LSASS = "installer.exe"
BANG_INSTALLER_PAM = "installer"

# stamped configuration values - found and replaced in the installer binary
BANG_MODULE_NAME_STAMP = "BASKETBALLJONES"
BANG_HOSTNAME_STAMP = "EVERYBODYGETUP"
BANG_AUTH_TOKEN_STAMP = "00000000-0000-0000-0000-000000000000"


def _stamp_value(binary: bytes, stamp_pattern: bytes, stamp_data: bytes) -> bytes:
    idx = binary.index(stamp_pattern)
    return binary.replace(
        binary[idx : idx + len(stamp_data) + 1],
        stamp_data + b"\x00",
        1,
    )


def configure_bang_installer(
    debug: bool, hostname: str, auth_token: str, module_name: str, target: str
) -> bytes:
    configuration = "debug" if debug else "release"
    filename = BANG_INSTALLER_LSASS if target == "lsass" else BANG_INSTALLER_PAM
    encoding = "utf-16-le" if target == "lsass" else "utf-8"

    installer_path = EXPORT_DIR / target / configuration / filename
    with open(installer_path, "rb") as f:
        installer_bin = f.read()

    installer_bin = _stamp_value(installer_bin, BANG_HOSTNAME_STAMP.encode(encoding), hostname.encode(encoding))
    installer_bin = _stamp_value(installer_bin, BANG_AUTH_TOKEN_STAMP.encode(encoding), auth_token.encode(encoding))
    if target == "lsass":
        installer_bin = _stamp_value(installer_bin, BANG_MODULE_NAME_STAMP.encode(encoding), module_name.encode(encoding))

    return installer_bin


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--module-name", default="bang", help="Name for the module (does not include file extension)")
    parser.add_argument("--hostname", required=True, help="HTTPS hostname")
    parser.add_argument(
        "--auth-token", default="00000000-0000-0000-0000-000000000000", help="API token (UUID)"
    )
    parser.add_argument(
        "-t",
        "--target",
        required=True,
        choices=["lsass", "pam"],
        help="Type of module to configure",
    )
    parser.add_argument("-d", "--debug", action="store_true", help="Configure debug binaries")
    parser.add_argument(
        "-o",
        "--output",
        type=pathlib.Path,
        help="Configured installer output path",
    )
    args = parser.parse_args()
    if args.output:
        out_path = args.output
    else:
        out_path = pathlib.Path(BANG_INSTALLER_LSASS if args.target == "lsass" else BANG_INSTALLER_PAM)
    output_bin = configure_bang_installer(
        debug=args.debug,
        hostname=args.hostname,
        auth_token=args.auth_token,
        module_name=args.module_name,
        target=args.target,
    )
    with out_path.open("wb") as f:
        f.write(output_bin)


if __name__ == "__main__":
    main()
