from importlib import resources

from . import installers

INSTALLER_DIR = resources.files(installers)

BUPKUS_INSTALLER = INSTALLER_DIR / "bupkus-installer.exe"

# stamped configuration values - found and replaced in the installer binary
BUPKUS_MODULE_NAME_STAMP = "BASKETBALLJONES"
BUPKUS_HOSTNAME_STAMP = "EVERYBODYGETUP"
BUPKUS_AUTH_TOKEN_STAMP = "00000000-0000-0000-0000-000000000000"


def _stamp_value(binary: bytes, stamp_pattern: bytes, stamp_data: bytes) -> bytes:
    idx = binary.index(stamp_pattern)
    return binary.replace(
        binary[idx : idx + len(stamp_data) + 1],
        stamp_data + b"\x00",
        1,
    )


def configure_bupkus_installer(hostname: str, auth_token: str, listener_name: str) -> bytes:
    with BUPKUS_INSTALLER.open("rb") as f:
        installer_bin = f.read()

    installer_bin = _stamp_value(
        installer_bin,
        BUPKUS_HOSTNAME_STAMP.encode("utf-16-le"),
        hostname.encode("utf-16-le"),
    )
    installer_bin = _stamp_value(
        installer_bin,
        BUPKUS_AUTH_TOKEN_STAMP.encode("utf-16-le"),
        auth_token.encode("utf-16-le"),
    )
    installer_bin = _stamp_value(
        installer_bin,
        BUPKUS_MODULE_NAME_STAMP.encode("utf-16-le"),
        listener_name.encode("utf-16-le"),
    )

    return installer_bin, BUPKUS_INSTALLER.name
