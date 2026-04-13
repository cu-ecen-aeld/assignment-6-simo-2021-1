# !/bin/bash
# Script to open qemu terminal.
# Author: Siddhant Jajoo.

# Emulate QEMU on an ARM64 system by using RAMDISK 
# Goal: Boot the rootfs from RAM (temporary, non-persistent storage)


set -e

OUTDIR=$1

if [ -z "${OUTDIR}" ]; then
    OUTDIR=/tmp/aeld
    echo "No outdir specified, using ${OUTDIR}"
    echo "try this outdir:  ./start-qemu-terminal.sh /home/tchuinkou/aeld"
fi

KERNEL_IMAGE=${OUTDIR}/Image
INITRD_IMAGE=${OUTDIR}/initramfs.cpio.gz


#read -p "Appuie sur Entrée pour continuer..."
echo "		"

if [ ! -e ${KERNEL_IMAGE} ]; then
    echo "Missing kernel image at ${KERNEL_IMAGE}"
    read -p "Appuie sur Entrée pour continuer..."
    echo "		"
    exit 1
fi

if [ ! -e ${INITRD_IMAGE} ]; then
    echo "Missing initrd image at ${INITRD_IMAGE}"
#    read -p "Appuie sur Entrée pour continuer..."
    echo "		"
    exit 1
fi

echo "Start QEMU Emulation"
#read -p "Appuie sur Entrée pour continuer..."
echo "		"

# See trick at https://superuser.com/a/1412150 to route serial port output to file
qemu-system-aarch64 -m 256M -M virt -cpu cortex-a53 -nographic -smp 1 -kernel ${KERNEL_IMAGE} \
        -chardev stdio,id=char0,mux=on,logfile=${OUTDIR}/serial.log,signal=off \
        -serial chardev:char0 -mon chardev=char0\
        -append "rdinit=/bin/sh" -initrd ${INITRD_IMAGE}
        
        #Uses kernelimage in zlmekernelbootloader)(bypassingend" to the kernel.Passes argments # in"applescribe virtuaUses a devicetree file to (hardware cthe kernel.onnections to
