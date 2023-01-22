"""Come on and SLAM! And welcome to the JAM!"""
import argparse
import glob
import os
import pathlib
import shutil
import subprocess

ROOT_DIR = pathlib.Path(__file__).parent
BUILD_DIR = ROOT_DIR / "build"
KERNEL_DIR = ROOT_DIR / "kernel"
USER_DIR = ROOT_DIR / "user"

DEFAULT_EXE_NAME = "monstars"
DEFAULT_KO_NAME = "monstars_nf"

KO_PRECOMPILED_H = "ko_precompiled.h"

KERNEL_VERSIONS = [
    "3.11.0-12-generic",
    "3.13.0-24-generic",
    "4.4.0-21-generic",
    "4.15.0-20-generic",
    "5.15.0-56-generic",
]

KERNEL_C_FLAGS = {
    "3": "-D__GNUC__=4 -fno-pie -Wno-pointer-sign -Wno-attributes -Wno-missing-attributes -mfentry",
    "4": "-fno-pie -Wno-pointer-sign -Wno-attributes",
}


def run_build(kernel_ver, ko_name, exe_name, debug):
    """Build it!"""
    target = "debug" if debug else ""
    os.environ.update(
        {
            "KERNEL_VER": kernel_ver,
            "MOD": ko_name,
            "EXE": exe_name,
            "FLAGS": KERNEL_C_FLAGS.get(kernel_ver[0], ""),
        }
    )
    # Cleanup old build artifacts
    if os.path.isdir(BUILD_DIR):
        shutil.rmtree(BUILD_DIR)
    os.mkdir(BUILD_DIR)

    # Build kernel module
    print(f"\n\n** Building {ko_name}.ko for {kernel_ver}...\n")
    tempname = (ko_name != DEFAULT_KO_NAME)
    if tempname:
        shutil.copyfile(f"{KERNEL_DIR}/{DEFAULT_KO_NAME}.c", f"{KERNEL_DIR}/{ko_name}.c")
    subprocess.run(
        f"make {target}", cwd=KERNEL_DIR, shell=True
    )
    # Cleanup kbuild
    if tempname:
        os.unlink(f"{KERNEL_DIR}/{ko_name}.c")
    subprocess.run(
        "make clean", cwd=KERNEL_DIR, shell=True
    )
    # Generate precompiled .ko header
    print(f"\n\n** Generating {KO_PRECOMPILED_H} for {kernel_ver}...")
    with open(f"{BUILD_DIR}/{ko_name}-{kernel_ver}.ko", "rb") as f:
        bin_str = bytes.hex(f.read())
    with open(BUILD_DIR / f"{KO_PRECOMPILED_H}", "w") as hdr:
        hdr.write(
            "/*\n"
            " *  This is a generated header file.\n"
            " */"
        )
        # static const byte c_KernelMod[4944] = {0x7f, 0x45, 0x4c, 0x46, ... };
        arr_elems = [f"0x{bin_str[i:i+2]}" for i in range(0, len(bin_str), 2)]
        hdr.write(
            "\n\n"
            f"static const char c_KernelMod[{len(arr_elems)}] = {{{', '.join(arr_elems)}}};"
        )

    # Build userland executable
    print(f"\n\n** Building {exe_name} for {kernel_ver}...\n")
    subprocess.run(
        f"make {target}", cwd=USER_DIR, shell=True
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
    if args.ko_name == DEFAULT_KO_NAME and not args.debug:
        print("*** WARNING: USED DEFAULT KO NAME FOR NON-DEBUG BUILD ***")
    if args.exe_name == DEFAULT_EXE_NAME and not args.debug:
        print("*** WARNING: USED DEFAULT EXE NAME FOR NON-DEBUG BUILD ***")
    run_build(args.kernel_ver, args.ko_name, args.exe_name, args.debug)
