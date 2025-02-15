import argparse
import pathlib
import subprocess

ROOT_DIR = pathlib.Path(__file__).resolve().parent.parent.parent
EXPORT_DIR = ROOT_DIR / "_export" / "bang"

APP_SRC_DIR = ROOT_DIR / "bang" / "app"

BANG_INSTALLER_LSASS = "bang-installer.exe"
BANG_INSTALLER_PAM = "bang-installer"


def _create_symlink(link_path: pathlib.Path, target_path: pathlib.Path):
    if not target_path.exists():
        print(f"** WARNING: {target_path} does not exist **")
    link_path.parent.mkdir(parents=True, exist_ok=True)
    link_path.unlink(missing_ok=True)
    link_path.symlink_to(target_path)


def create_symlinks(debug: bool):
    config = "debug" if debug else "release"
    _create_symlink(
        APP_SRC_DIR / "bang" / "installers" / "lsass" / BANG_INSTALLER_LSASS,
        EXPORT_DIR / "lsass" / config / BANG_INSTALLER_LSASS,
    )
    _create_symlink(
        APP_SRC_DIR / "bang" / "installers" / "pam" / BANG_INSTALLER_PAM,
        EXPORT_DIR / "pam" / config / BANG_INSTALLER_PAM,
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--debug", action="store_true", help="package debug installer binaries")
    args = parser.parse_args()
    create_symlinks(args.debug)
