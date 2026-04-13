#!/bin/bash
# Script to run QEMU for buildroot as the default configuration qemu_aarch64_virt_defconfig

# Emulate QEMU on an ARM64 system by using emulated SDD (HDD path) 
# Goal: Boot the rootfs from the emulated SDD (persistent storage)

# Host forwarding: Host Port 10022 ->> QEMU Port 22 
# Author: Siddhant Jajoo.
# Modified by Arnaud, ER den 28.01.2026
# # Host forwarding: ostfwd=tcp::9000-:9000
# 30JAN2026
# Script to run QEMU with CAN virtual bus

# === Configuration CAN virtuel ===
# Créer un bus CAN virtuel sur l'hôte (vcan0)
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

# Ajouter le forwarding CAN entre hôte et QEMU
QEMU_CAN_ARGS="-netdev socket,id=can0,can=on,host=127.0.0.1,port=54711 -device virtio-can-device,netdev=can0"


qemu-system-aarch64 \
    -M virt  \
    -cpu cortex-a53 -nographic -smp 1 \
    -kernel buildroot/output/images/Image \
    -append "rootwait root=/dev/vda console=ttyAMA0" \
    -netdev user,id=eth0,hostfwd=tcp::10022-:22,hostfwd=tcp::9000-:9000,hostfwd=tcp::9001-:9001 \
    -device virtio-net-device,netdev=eth0 \
    -drive file=buildroot/output/images/rootfs.ext4,if=none,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0 -device virtio-rng-pci
