# Linux headers for Monstars builder

These tarballs were scavenged from official `apt` and `yum` packages. The packages were downloaded either directly via package manager, or by directly browsing repo mirrors and archives to find the required versions.

## Installation

TODO

## Adding kernels

### .deb packages

```bash
dpkg-deb -xv ./linux-headers-a.b.c-d.deb .
tar -cvjf ./linux-headers-a.b.c-d.tar.gz -C ./usr/src linux-headers-a.b.c-d
```