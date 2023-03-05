# monstars netfilter

**monstars_nf** is a linux backdoor intended to be compatible with a wide range of kernel versions. It is made up of two main components: a user-mode executable and a kernel module. An installer binary must be run as `root` to install and persist the backdoor on the target system.

While kernel and distro agnosticism is the eventual goal, the backdoor has currently only been tested to work on:
 - `3.10.0-123.el7.x86_64`
 - `3.10.0-517.el7.x86_64`
 - `4.15.0-122-generic`
 - `4.15.0-123-generic`
 - `4.15.0-204-generic`

The backdoor currently supports the following commands:
 - `PING` (check for responsiveness)
 - `GET` (retrieve a file)
 - `EXEC` (run a shell command)

Commands are sent using UDP messages, and responses from the backdoor are sent over TCP connections. Commands and response data are Base64-encoded, but not encrypted.

----------

## Kernel Module

During initialization, the kernel module installs a netfilter hook to listen for commands sent by the controller. "Magic" UDP packets with a source port of `31337` have their data copied to a static buffer and are then dropped. All other packets are sent along untouched.

The module also creates a kernel worker thread named `kworker/l:1`, which checks for new task data once per second. If the static command buffer contains data, the kworker starts the user-mode executable in a new process, to handle the task in user-mode. The kworker thread is visible in `top` or `ps` output.

The file `/proc/task` is registered by the kernel module, and the user-mode executable accesses the received commands by reading from it.

During installation, the installer binary registers the kernel module to be loaded on boot, so that the backdoor is persistent across system reboots.

----------

## User-Mode Executable

The user-mode executable is spawned by the kernel worker thread whenever a new task is received. It opens and reads the `/proc/task` file, and parses the command data that was sent by the controller.

When the task is complete, the executable opens a TCP connection back to the controller, based on the source IP in the magic packet and the desired port number sent along with the command. The result of the command (such as file data or a PING response) is sent back over the TCP connection.

----------

## Installer Executable

The installer binary must be run with `root` permissions to successfully install the backdoor. It creates or modifies the following files:
 - The user-mode executable (path and name configurable), e.g., `/bin/monstars`
 - The kernel module (name configurable), e.g., `/lib/modules/<kernel version>/kernel/drivers/net/monstars_nf.ko`
 - A kernel module load config entry in one of these places:
    - `/etc/modules` (Ubuntu)
    - `/etc/modules-load.d/monstars_nf.conf` (CentOS, name configurable)

After dropping the files to disk, the installer invokes `depmod` to register the kernel module to be loaded on boot.

After all of the files are in place and the kernel module is registered, the installer finally inserts the kernel module itself, enabling the backdoor.

----------

## Controller

The controller is a Python module that can be installed as a wheel or in editable "develop" mode. It requires at least Python 3.7 in its environment.

The primary means of controlling a backdoor is the `call-monstars` CLI (run `call-monstars <command> -h` for command-specific options):
```
usage: call-monstars [-h] -i IP [-d DEST_PORT] [-l LISTEN_PORT] {ping,get,exec} ...

send it!

positional arguments:
  {ping,get,exec}       [supported commands]
    ping                ping a target
    get                 retrieve a file
    exec                run a system command

optional arguments:
  -h, --help            show this help message and exit
  -i IP, --ip IP        IP address of host machine
  -d DEST_PORT, --dest-port DEST_PORT
                        destination port number
  -l LISTEN_PORT, --listen-port LISTEN_PORT
                        TCP port to listen on for a response
```

For example, the PING command can be used to verify that a backdoor has been installed correctly:
```
(env) C:\Users\j_spa\Projects\git\monstars-netfilter>call-monstars -i 172.20.103.108 ping
sending magic packet --> 172.20.103.108:53
waiting for reply...
Received PONG from 172.20.103.108
```

The backdoor will read the source IP address from any "magic" packet it receives, so the controller can be used from any host with a route to the target machine.

In addition to the CLI, the Python module can also be imported and used as an API to script or automate the backdoors as part of a command-and-control server. The API should be considered in early development and subject to change drastically in the future.

----------

## C2 Web Server

The C2 server is a barebones Django application. The view models wrap the Python API, providing a nostalgic user interface for controlling netfilter backdoors installed on various systems.

----------

## Building an Installer

To build the installer, you will need kernel headers for the version of linux you want to target. Depending on your build environment, this may be challening to configure.

Once your build environment is set up, however, use the Python script `build_monstars.py` to build an installer specific to your target environment. The following attributes are configurable:
 - kernel object name
 - target kernel version
 - debugging output

The configured installer will be placed in the `./export/` directory, and will be named `go-monstars_<kernel version>`.
