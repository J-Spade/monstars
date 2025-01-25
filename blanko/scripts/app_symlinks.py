import argparse
import pathlib
import subprocess

ROOT_DIR = pathlib.Path(__file__).resolve().parent.parent.parent
EXPORT_DIR = ROOT_DIR / "_export" / "blanko"

APP_SRC_DIR = ROOT_DIR / "blanko" / "app"

BLANKO_INSTALLER = "blanko-install"


def _create_symlink(link_path: pathlib.Path, target_path: pathlib.Path):
    if not target_path.exists():
        print(f"** WARNING: {target_path} does not exist **")
    link_path.parent.mkdir(parents=True, exist_ok=True)
    link_path.unlink(missing_ok=True)
    link_path.symlink_to(target_path)


def create_symlinks(debug: bool):
    config = "debug" if debug else "release"
    for kernel in [k.name for k in EXPORT_DIR.iterdir()]:
        _create_symlink(
            APP_SRC_DIR / "blanko" / "installers" / kernel / BLANKO_INSTALLER,
            EXPORT_DIR / kernel / config / BLANKO_INSTALLER,
        )


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-d", "--debug", action="store_true", help="package debug installer binaries")
    args = parser.parse_args()
    create_symlinks(args.debug)
