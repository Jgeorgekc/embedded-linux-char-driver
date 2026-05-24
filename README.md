# Linux Character Device Driver

Custom Linux character device driver developed in C.

## Features
- Character device registration
- Read/Write operations
- Device node creation
- Kernel module loading/unloading
- User-space communication

## Technologies
- Linux Kernel
- C Programming
- QEMU
- BusyBox
- U-Boot

## Build
make

## Load Driver
sudo insmod chardriver_auto.ko

## Test
echo "hello" | sudo tee /dev/mydevice
sudo cat /dev/mydevice
