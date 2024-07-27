"""Come on and SLAM! And welcome to the JAM!"""
import argparse
import glob
import os
import pathlib
import shutil
import subprocess
import sys

# source file and build output paths
SRC_DIR = pathlib.Path(__file__).parent.parent / "src"
INSTALLER_SRC = SRC_DIR / "installer"
KERNEL_SRC = SRC_DIR / "kernel"
USER_SRC = SRC_DIR / "user"

ROOT_DIR = pathlib.Path(__file__).parent.parent.parent
BUILD_DIR = ROOT_DIR / "_build" / "blanko"
EXPORT_DIR = ROOT_DIR / "_export" / "blanko"

# binary artifact names
USER_EXE_NAME = "blanko"
INSTALLER_NAME = "blanko-install"

# kernel object name
KO_NAME_DEBUG = "blanko"
KO_NAME_RELEASE = "net_hid"

# precompiled headers and stampable constants
KO_PRECOMPILED_H = "ko_precompiled.h"
KO_CONST_NAME = "c_KernelMod"
USER_PRECOMPILED_H = "user_precompiled.h"
USER_CONST_NAME = "c_UserExe"

PRECOMPILED_HEADER_TEMPLATE = """
/*
 *  THIS IS A GENERATED HEADER FILE
 */
#ifndef {define}
#define {define}

char {var_name}[{arr_len}] = {{ {init_list} }};

#endif  // {define}
"""


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


def run_build(kernel_ver, debug):
    """Build it!"""
    ko_name = KO_NAME_DEBUG if debug else KO_NAME_RELEASE
    target = "debug" if debug else "release"
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
    if os.path.isdir(BUILD_DIR / kernel_ver / target):
        shutil.rmtree(BUILD_DIR / kernel_ver / target)
    os.makedirs(BUILD_DIR / kernel_ver / target, exist_ok=True)

    # Build kernel module
    print(f"\n\n** Building {ko_name}.ko for {kernel_ver}...\n")
    tempname = (ko_name != KO_NAME_DEBUG)
    if tempname:
        shutil.copyfile(KERNEL_SRC / f"{KO_NAME_DEBUG}.c", KERNEL_SRC / f"{ko_name}.c")
    subprocess.run(
        f"make {target}", cwd=KERNEL_SRC, shell=True
    )
    # Cleanup kbuild
    if tempname:
        os.unlink(KERNEL_SRC / f"{ko_name}.c")
    subprocess.run(
        "make clean", cwd=KERNEL_SRC, shell=True
    )

    # Build userland executable
    print(f"\n\n** Building user exe for {kernel_ver}...\n")
    subprocess.run(
        f"make {target}", cwd=USER_SRC, shell=True
    )

    # Generate precompiled headers
    with open(BUILD_DIR / kernel_ver / target / f"{ko_name}.ko", "rb") as f:
        ko_bin = f.read()
    _generate_header(
        ko_bin,
        BUILD_DIR / kernel_ver / target / KO_PRECOMPILED_H,
        KO_CONST_NAME,
    )
    with open(BUILD_DIR / kernel_ver / target / USER_EXE_NAME, "rb") as f:
        user_bin = f.read()
    _generate_header(
        user_bin,
        BUILD_DIR / kernel_ver / target / USER_PRECOMPILED_H,
        USER_CONST_NAME,
    )

    # Build installer executable
    print(f"\n\n** Building installer for {kernel_ver}...\n")
    os.makedirs(EXPORT_DIR / kernel_ver / target, exist_ok=True)
    subprocess.run(
        f"make {target}", cwd=INSTALLER_SRC, shell=True
    )


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
    parser = argparse.ArgumentParser(description="Build a blanko installer binary")
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
    args = parser.parse_args()
    # build
    run_build(args.kernel_ver, args.debug)
    print(f"\n\n** done!\n")
