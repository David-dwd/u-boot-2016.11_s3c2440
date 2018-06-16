#!/bin/bash
echo "Clean Configuration File..."
make distclean
echo "Clean Obj..."
make clean
echo "Load Configuration File..."
make smdk2440_defconfig
echo "make..."
make 
