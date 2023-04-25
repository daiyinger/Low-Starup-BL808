#!/bin/bash
set -e

LINUX_CROSS_PREFIX=riscv64-unknown-linux-gnu-

SHELL_DIR=$(cd "$(dirname "$0")"; pwd)
OUT_DIR=$SHELL_DIR/out

echo "start build spl"

cd $SHELL_DIR/spl/spl-project
make -j$(nproc)
cp -rf $SHELL_DIR/spl/spl-project/d0_spl/build/build_out/spl_bl808_d0.bin $OUT_DIR/spl_bl808_d0.bin

echo "start build opensbi"

cd $SHELL_DIR/opensbi-v0.6
make PLATFORM=thead/c910 CROSS_COMPILE=$LINUX_CROSS_PREFIX -j$(nproc)
cp -rf $SHELL_DIR/opensbi-v0.6/build/platform/thead/c910/firmware/fw_jump.bin $OUT_DIR/opensbi_v0.6.bin
