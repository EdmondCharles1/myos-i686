#!/bin/bash

if [ ! -f "kernel/build/myos.iso" ]; then
    echo "Error: ISO not found. Run 'make' in kernel/ first."
    exit 1
fi

qemu-system-i386 -cdrom kernel/build/myos.iso -m 128M
