from importlib import resources
import os
from typing import List

from .static.blanko import binaries

INSTALLER_DIR = resources.files(binaries)
INSTALLER_NAME = "blanko-install"

STAMP_MAGIC_USER_PATH = b"BASKETBALLJONES"
STAMP_MAGIC_KO_PATH = b"MORONMOUNTAIN"


def _stamp_value(binary: bytes, stamp_pattern: bytes, stamp_data: bytes) -> bytes:
    idx = binary.index(stamp_pattern)
    return binary.replace(
        binary[idx : idx + len(stamp_data) + 1],
        stamp_data + b"\x00",
        1,
    )


def available_kernels() -> List[str]:
    return [ver.name for ver in INSTALLER_DIR.iterdir() if not ver.name.startswith(".")]


def configure_blanko_installer(kernel_ver: str, exe_path: str) -> bytes:
    with open(INSTALLER_DIR / kernel_ver / INSTALLER_NAME, "rb") as f:
        installer_bin = f.read()

    # userland exe path gets stamped twice (installer and contained precompiled kernel mod)
    installer_bin = _stamp_value(installer_bin, STAMP_MAGIC_USER_PATH, exe_path.encode("ascii"))
    installer_bin = _stamp_value(installer_bin, STAMP_MAGIC_USER_PATH, exe_path.encode("ascii"))
    installer_bin = _stamp_value(
        installer_bin,
        STAMP_MAGIC_KO_PATH,
        f"/lib/modules/{kernel_ver}/kernel/drivers/net/net_hid.ko".encode("ascii"),
    )
    return installer_bin
