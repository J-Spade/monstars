# blanko

**blanko** is a linux backdoor intended to be compatible with a wide range of kernel versions. It is made up of two main components: a user-mode executable and a kernel module. An installer binary must be run as `root` to install and persist the backdoor on the target system.

The backdoor currently supports the following commands:
 - `PING` (check for responsiveness)
 - `GET` (retrieve a file)
 - `EXEC` (run a shell command)
 - `SHELL` (connect a reverse TCP shell to a listening host)

Commands are sent using UDP messages with a source port of `31337`, and responses are sent over TCP connections back to the sender. Commands and response data are base64-encoded, but not encrypted.

----------

## Installer

To build an installer, you will need kernel headers for the version of linux you want to target. Obtaining and installing the correct build tools for your kernel target is left as an exercise for the reader.

Once your build environment is set up, simply use the Python script `build_monstars.py` to build an installer specific to your target environment. The following attributes are configurable at build time:
 - kernel object name
 - target kernel version
 - debug configuration (logging and symbols)

The configured installer will be placed in the `./export/` directory, and will be named `go-monstars_<kernel version>`.

The installer binary must be run with `root` permissions to successfully install the backdoor. It creates and/or modifies the following files:
 - The user-mode executable (path and name configurable), e.g., `/bin/monstars`
 - The kernel module, e.g., `/lib/modules/4.15.0-123-generic/kernel/drivers/net/monstars_nf.ko`
 - A kernel module load config entry in one of these places:
    - `/etc/modules` (Ubuntu)
    - `/etc/modules-load.d/monstars_nf.conf` (CentOS)

All dropped and modified files have their `mtime` timestamp adjusted to match nearby files with similar names. After dropping the files to disk, the installer invokes `depmod` to register the kernel module to be loaded on boot.

Once all of the files are in place and the kernel module is registered, the installer finally inserts the kernel module itself, enabling the backdoor. Once inserted, the module is visible in `lsmod` output.

----------

## Controller

The controller is a Python module that can be installed as a wheel or in editable "develop" mode. It requires at least Python 3.7 in its environment.

The `call-monstars` CLI can be used to send commands to an installed backdoor. For example, the PING command can be used to verify that a backdoor has been installed correctly:
```
(env) C:\Users\j_spa\Projects\git\monstars-netfilter>call-monstars -i 172.20.103.108 ping
sending magic packet --> 172.20.103.108:53
waiting for reply...
Received PONG from 172.20.103.108
```
Run `call-monstars -h` to see a list of commands, or `call-monstars <command> -h` for command-specific options.

In addition to the CLI, the Python module can also be imported and used as an API to script or automate the backdoors.

----------

## Behavior

During initialization, the kernel module installs a netfilter hook to listen for commands sent by the controller. "Magic" UDP packets with a source port of `31337` have their data copied to a static buffer and are then dropped. All other packets are sent along untouched by the filter.

To process commands, the kernel module registers the the file `/proc/task` and spins up a worker thread named `kworker/l:1`. If the static command buffer contains data, the worker thread spawns a new process to handle the command in user-mode. The user-mode executable opens and reads the `/proc/task` file, retrieving the command data from the kernel module.

Once the task is complete, the user-mode executable opens a TCP connection back to the controller, using the source IP from the "magic" packet, and the port number sent with the command. The result of the command (e.g., file data, a PING response) is sent back over the TCP connection.

Neither the `kworker/l:1` kernel thread nor the `/proc/task` file are hidden from by any sort of rootkit trickery; they're visible in `top` and `ls /proc` output, respectively.

----------

## License

Base64 implementation Copyright (c) 2002-2012, Jouni Malinen <j@w1.fi> and contributors
All Rights Reserved.

This software may be distributed, used, and modified under the terms of BSD license:

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name(s) of the above-listed copyright holder(s) nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.