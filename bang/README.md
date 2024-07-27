# bang

**bang** for Windows (based on a proof-of-concept by [Red Team Notes](https://www.ired.team/offensive-security/credential-access-and-credential-dumping/intercepting-logon-credentials-via-custom-security-support-provider-and-authentication-package)) is a Security Support Provider (SSP) that sends intercepted logon credentials from the Local Security Authority (LSA) to an HTTPS endpoint.

**bang** for Linux is a Pluggable Authentication Modules (PAM) module that sends intercepted logon credentials from the PAM authentication stack to an HTTPS endpoint.

**TODO:**
* automated uninstallers (without reboot, if possible)
* implement PAM module


## Building

### Windows
The recommended build environment is Visual Studio 2022. Building the solution `bang.sln` will produce the SSP DLL and an installer EXE under the `export/` directory.

The SSP DLL is packaged as a resource into the installer EXE; on its own, the DLL is not required unless installing the SSP manually.

TLS/HTTPS is enabled for the SSP by default; to turn it off (for Django development or other testing), override the `BANG_TLS_ENABLED` compiler definition (by command line or by editing `provider/src/bang_http.cpp`).

### Linux
TODO


## Django App

The `bang` Python package is a Django app used for handling `POST` requests sent by the SSP. The web interface can also be used to:
* view collected logon credentials
* list and/or revoke API tokens
* configure an installer EXE for deployment to a target

Usage of the web interfaces requires a user account on the Django webserver.

Before the webapp can be used to configure an installer, the compiled base installer EXE must be copied/moved from the `export/` directory into `app/bang/static/bang/binaries/`.


## Helper Scripts

A few helper scripts are provided for convenience when testing outside of a Django environment.

### configure.py
`configure.py` is a command-line interface for configuring an installer binary. It assumes compiled binaries are present in the `export/` directory (see [Building](#building)).

### server.py
The script bangsrv.py is capable of listening for credentials sent by the provider. The script requires a `bangsrv.pem` file in the same directory, containing a TLS server certificate and private key.


## Installation

### Windows
Run the configured installer EXE on the target. The installer handles:
* placing the SSP DLL in `C:\Windows\System32`
* transfering ownership of the SSP DLL to `TrustedInstaller`
* readjusting the created/modified/accessed timestamps for the SSP DLL and parent directory
* registering the SSP with LSA (no reboot required)

These operations require elevated privileges.

### Linux
TODO


## Uninstallation

### Windows
Uninstalling the SSP is currently a manual process:
1. Remove the SSP entry from the list of SSPs in the `Security Packages` value of the registry key `HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Lsa`.
2. Reboot the target machine
3. Delete the SSP DLL from `C:\Windows\System32\`

### Linux
TODO