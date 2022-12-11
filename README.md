# monstars netfilter

TODO: description

## Building a Payload

**$ python3 build_monstars.py**
```
usage: build_monstars.py [-h] [-d] [-e EXE_NAME] [-v {5.15.0-56-generic}] [-k KO_NAME]

summon the monstars!

optional arguments:
  -h, --help            show this help message and exit
  -d, --debug           build components with debug symbols
  -e EXE_NAME, --exe-name EXE_NAME
                        name to use for the userland executable
  -v {5.15.0-56-generic}, --kernel-ver {5.15.0-56-generic}
                        linux kernel version to build against
  -k KO_NAME, --ko-name KO_NAME
                        name to use for the kernel netfilter module
```

## Shooting Hoops

**$ python3 call_monstars.py**
```
usage: call_monstars.py [-h] -i IP [-p {tcp,udp}] [-d DEST_PORT] {ping}

send it!

positional arguments:
  {ping}                command to send

optional arguments:
  -h, --help            show this help message and exit
  -i IP, --ip IP        IP address of host machine
  -p {tcp,udp}, --proto {tcp,udp}
                        transport-layer protocol to use
  -d DEST_PORT, --dest-port DEST_PORT
                        destination port number
```
