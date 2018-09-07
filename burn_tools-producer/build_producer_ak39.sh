#!/bin/sh

cd `dirname $0`
PWD=`pwd`
CURRENT=$PWD
PRODUCER=$PWD/producer_all
FHA=$PWD/producer_fha
BUILD=$PWD/build
ROOTFS=$PWD/../../../platform/rootfs
KERNEL=$PWD/../../../kernel
cd $CURRENT

echo $ROOTFS
echo $KERNEL
echo $PRODUCER
echo $FHA
echo $BUILD
echo $CURRENT

cd $ROOTFS
sudo rm -rf $CURRENT/rootfs
sudo tar xvzf striped_rootfs.tar.gz -C $CURRENT

cd $FHA
chmod +x build_gcc.bat
./build_gcc.bat

cd $PRODUCER
chmod +x build_ak39.sh
./build_ak39.sh

cd $KERNEL
if [ ! -e $BUILD ]
then
	mkdir $BUILD
	make ak39_producer_defconfig O=$BUILD
fi
rm $KERNEL/lib/libfha.a -rf
cp $FHA/fha.a $KERNEL/lib/libfha.a
make O=$BUILD -j4
sudo cp $BUILD/drivers/usb/gadget/g_file_storage.ko $CURRENT/rootfs/root
sudo cp $PRODUCER/producer $CURRENT/rootfs/root
make O=$BUILD -j4

cp $BUILD/arch/arm/boot/zImage $CURRENT/producer_ak39.bin
sudo rm -rf $CURRENT/rootfs
sudo rm -rf $BUILD

