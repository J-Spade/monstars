# bupkus

**bupkus** is a Windows executable that registers a clipboard notification routine, and sends any text found on the clipboard to an HTTPS endpoint.

## Building

The recommended build environment is Visual Studio 2022. Building the solution `monstars.sln` will create an installer EXE within the `_export/bupkus` directory of the repo.

The listener executable is packaged as a resource into the installer EXE, or can also be located within the `_build/bupkus` directory of the repo.


## Django App

The `bupkus-swackhammer` Python package is a Django app used for handling `POST` requests sent by the SSP. The web interface can also be used to:
* view collected clipboard data
* list and/or revoke API tokens
* configure an installer EXE for deployment to a target

Usage of the web interface requires a user account on the Django webserver.

Before the webapp can be used to configure an installer, compiled installer binaries must be copied/moved from the `_export/` directory into `app/bupkus/installers/`.

### Packaging
For deployment, the `bupkus-swackhammer` package should be built as a Python wheel for easy installation in the webserver Django environment:
1. Build the un-configured installer binaries (see [Building](#building))
1. Use the `app_symlinks.py` helper script to generate installer symlinks within the local sourcetree (see [Helper Scripts](#app_symlinkspy))
1. Use the `build` Python package to build the wheel:
    ```bash
    $ python3 -m pip install build
    $ python3 -m build ./bupkus/app --wheel --outdir ./_export
    ```

## Helper Scripts

A few helper scripts are provided at `bupkus/scripts/`, for convenience when testing outside of a Django environment.

### app_symlinks.py
`app_symlinks.py` is a utility for creating symlinks to compiled installer binaries within the Django app package. It assumes compiled binaries are present in the `_export/` directory (see [Building](#building)) but will create symlinks even if no binaries exist.

Once symlinks have been created, the app sourcetree is usable as an editable python package. Symlinks are also followed when packaging the webapp for deployment (see [Django App: Packaging](#packaging)).

### configure.py
`configure.py` is a command-line interface for configuring an installer binary. It requires compiled binaries to be present in the `_export/` directory (see [Building](#building)).

### server.py
`server.py` is a simple HTTPS server capable of listening for credentials sent by the provider. The script requires a `bupkus.pem` file in the same directory, containing a TLS server certificate and private key. Credentials are printed to the console, and not otherwise saved anywhere.


## Installation

Run the configured installer EXE on the target. The installer handles:
* placing the listener EXE in `C:\Windows\System32`
* transfering ownership of the EXE to `TrustedInstaller`
* readjusting the created/modified/accessed timestamps for the EXE and parent directory
* registering the listener EXE as an on-exit debugger for the `userinit.exe` Windows process

These operations require elevated privileges.
