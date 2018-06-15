#!/bin/bash
echo "Clean Configuration File..."
make distclean
echo "Clean Obj..."
make clean
echo "Load Configuration File..."
make smdk2440_defconfig
echo "make..."
make CROSS_COMPILE=/work/s3c2440/cross_tools/4.3.2/bin/arm-none-linux-gnueabi-
