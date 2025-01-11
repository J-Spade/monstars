"""Come on and SLAM! And welcome to the JAM!"""
import argparse
import glob
import os
import pathlib
import shutil
import subprocess
import sys

# source file and build output paths
SRC_DIR = pathlib.Path(__file__).parent.parent / "src" / "pam"
INSTALLER_SRC = SRC_DIR / "installer"
MODULE_SRC = SRC_DIR / "module"

ROOT_DIR = pathlib.Path(__file__).parent.parent.parent
BUILD_DIR = ROOT_DIR / "_build" / "bang" / "pam"
EXPORT_DIR = ROOT_DIR / "_export" / "bang" / "pam"

# binary artifact names
INSTALLER_NAME = "installer"
SO_NAME = "pam_bang.so"

# precompiled headers and stampable constants
MOD_PRECOMPILED_H = "mod_precompiled.h"
MOD_CONST_NAME = "c_PamModule"

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


def run_build(target):
    """Build it!"""
    os.environ.update(
        {
            "INSTALLER_NAME": INSTALLER_NAME,
            "SO_NAME": SO_NAME,
        }
    )
    # Clean up old build artifacts
    if os.path.isdir(BUILD_DIR / target):
        shutil.rmtree(BUILD_DIR / target)
    os.makedirs(BUILD_DIR / target, exist_ok=True)

    # Build PAM module
    print(f"\n\n** Building PAM module...\n")
    subprocess.run(
        f"make {target}", cwd=MODULE_SRC, shell=True
    )

    # Generate precompiled header
    with open(BUILD_DIR / target / SO_NAME, "rb") as f:
        mod_bin = f.read()
    _generate_header(
        mod_bin,
        BUILD_DIR / target / MOD_PRECOMPILED_H,
        MOD_CONST_NAME,
    )

    # Build installer executable
    print(f"\n\n** Building installer...\n")
    os.makedirs(EXPORT_DIR / target, exist_ok=True)
    subprocess.run(
        f"make {target}", cwd=INSTALLER_SRC, shell=True
    )


if __name__ == "__main__":
    # args
    parser = argparse.ArgumentParser(description="Build a blanko installer binary")
    parser.add_argument(
        "-d", "--debug",
        action="store_true",
        help="build components with debug symbols",
    )
    args = parser.parse_args()
    # build
    run_build("debug" if args.debug else "release")
    print(f"\n\n** done!\n")
