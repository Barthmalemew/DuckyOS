#!/bin/bash

set -xe

. ./setupenv.sh

cp duckyos.bin ./isodir/boot/
cp grub.cfg ./isodir/boot/grub/
grub2-mkrescue -o duckyos.iso isodir

