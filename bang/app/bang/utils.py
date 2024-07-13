from importlib import resources
import uuid

BANG_INSTALLER_PATH = resources.files("bang") / "static" / "bang" / "binaries" / "installer.exe"

# stamped configuration values - found and replaced in the installer binary
BANG_SSP_NAME_STAMP = "BASKETBALLJONES".encode("utf-16-le")
BANG_HOSTNAME_STAMP = "EVERYBODYGETUP".encode("utf-16-le")
BANG_AUTH_TOKEN_STAMP = "00000000-0000-0000-0000-000000000000".encode("utf-16-le")


def _stamp_value(binary: bytes, stamp_pattern: bytes, stamp_data: bytes) -> bytes:
    idx = binary.index(stamp_pattern)
    return binary.replace(binary[idx : idx + len(stamp_data) + 1], stamp_data + b"\x00")


def configure_bang_installer(hostname: str, auth_token: str, provider_name: str) -> bytes:
    with open(BANG_INSTALLER_PATH, "rb") as f:
        installer_bin = f.read()
    installer_bin = _stamp_value(installer_bin, BANG_HOSTNAME_STAMP, hostname.encode("utf-16-le"))
    installer_bin = _stamp_value(installer_bin, BANG_AUTH_TOKEN_STAMP, auth_token.encode("utf-16-le"))
    installer_bin = _stamp_value(installer_bin, BANG_SSP_NAME_STAMP, provider_name.encode("utf-16-le"))
    return installer_bin
