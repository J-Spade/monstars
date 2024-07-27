"""Come on and SLAM! And welcome to the JAM!"""
import argparse
import glob
import os
import pathlib
import shutil
import subprocess
import sys

ROOT_DIR = pathlib.Path(__file__).parent.parent.parent
EXPORT_DIR = ROOT_DIR / "_export" / "blanko"

# binary artifact names
INSTALLER_NAME = "blanko-install"

# kernel object name
KO_NAME_DEBUG = "blanko"
KO_NAME_RELEASE = "net_hid"

STAMP_MAGIC_USER_PATH = b"BASKETBALLJONES"
STAMP_MAGIC_KO_PATH = b"MORONMOUNTAIN"

DEFAULT_EXE_PATH = "/usr/bin/blanko"


def _stamp_binary(binary: bytes, stamp_pattern: bytes, stamp_data: bytes) -> bytes:
    """Stamp over a pattern in the binary data and return the result"""
    idx = binary.index(stamp_pattern)
    return binary.replace(
        binary[idx : idx + len(stamp_data) + 1],
        stamp_data + b"\x00",
        1,
    )


def configure(kernel_ver: str, exe_path: str, debug: bool) -> bytes:
    """stamp the installer with config data"""
    ko_name = KO_NAME_DEBUG if debug else KO_NAME_RELEASE
    target = "debug" if debug else "release"
    with open(EXPORT_DIR / kernel_ver / target / INSTALLER_NAME, "rb") as f:
        installer_bin = f.read()

    # userland exe path gets stamped twice (installer and contained precompiled kernel mod)
    installer_bin = _stamp_binary(installer_bin, STAMP_MAGIC_USER_PATH, exe_path.encode("ascii"))
    installer_bin = _stamp_binary(installer_bin, STAMP_MAGIC_USER_PATH, exe_path.encode("ascii"))
    installer_bin = _stamp_binary(
        installer_bin,
        STAMP_MAGIC_KO_PATH,
        f"/lib/modules/{kernel_ver}/kernel/drivers/net/{ko_name}.ko".encode("ascii"),
    )
    return installer_bin


if __name__ == "__main__":
    # autodetect installed kmod build environments
    try:
        kernel_versions = [
            ver for ver in os.listdir("/lib/modules") if os.path.exists(f"/lib/modules/{ver}/build")
        ]
        kernel_versions.sort()
    except FileNotFoundError:
        kernel_versions = []
    if len(kernel_versions) == 0:
        print("*** WARNING: NO KBUILD ENVIRONMENTS DETECTED ***")
    # args
    parser = argparse.ArgumentParser(description="Build a monstars_nf installer binary")
    parser.add_argument(
        "-d", "--debug",
        action="store_true",
        help="build components with debug symbols",
    )
    parser.add_argument(
        "-v", "--kernel-ver",
        default=kernel_versions[-1] if len(kernel_versions) else None,
        choices=kernel_versions,
        help="linux kernel version to build against",
    )
    parser.add_argument(
        "-e", "--exe-path",
        default=DEFAULT_EXE_PATH,
        help="path to the userland executable",
    )
    parser.add_argument(
        "-o", "--output",
        default="blanko-installer",
        help="configured installer output path",
        type=pathlib.Path,
    )
    args = parser.parse_args()
    if args.exe_path == DEFAULT_EXE_PATH and not args.debug:
        print("*** WARNING: USED DEFAULT EXE PATH FOR NON-DEBUG BUILD ***")
    # configure
    output_bin = configure(args.kernel_ver, args.exe_path, args.debug)
    with args.output.open("wb") as f:
        f.write(output_bin)
