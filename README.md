# Xoshiro 256

Xorshiro256 is a Linux kernel module based on davidscholberg/merandom. The main different is meradom usees xorshift algorithm
while Xoshiro256 uses Xoshiro 256 plus algorithm. This change gain a huge performance.

## Installation

Create .ko file using Kbuild method 

```bash
$ make
```

Then load driver to kernel as root user

```bash
$ sudo insmod xoshiro256
```

Make sure module already load

```bash
$ lsmod | grep xoshiro256
```

## Usage

Using dd command with parameter to read 16 byttes and output it in hex format

```bash
$ dd if=/dev/xoshiro256 bs=16 count=1 2>/dev/null | hexdump -C
```

## Uninstallation

Remove xoshiro256 as root user

```bash
$ sudo rmmod xoshiro256
```
