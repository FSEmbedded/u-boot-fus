#!/bin/bash

arch=$1
fert=$2

if [ "$arch" == "8mm" ]; then
	boardtype=0
	boardrev=130
	export DTB=picocoremx8mm.dtb
	if [ "$fert" == "fert3" ]; then
		features=0xE9;
		defconfig=fsimx8mm_defconfig
	fi
elif [ "$arch" == "8mx" ]; then
	boardtype=1
	boardrev=120
	export DTB=picocoremx8mx-selftest.dtb
	if [ "$fert" == "fert5" ]; then
		features=0xD4;
		defconfig=fsimx8mm_defconfig
	fi
	if [ "$fert" == "fert6" ]; then
		features=0x94
		defconfig=fsimx8mm_defconfig
	fi
	if [ "$fert" == "fert7" ]; then
		features=0x8B;
		defconfig=fsimx8mm_mmc_defconfig
	fi
	if [ "$fert" == "fert8" ]; then
		features=0x6B;
		defconfig=fsimx8mm_mmc_defconfig
	fi
else
	boardtype=0
	boardrev=100
	features=0x0
	defconfig=fsimx8mm_defconfig
fi

export CONFIG_FUS_BOARDTYPE=$boardtype;export CONFIG_FUS_BOARDREV=$boardrev;export CONFIG_FUS_FEATURES2=$features;

make $defconfig
make -j6
retval=$?

if [ $retval -ne 0 ]; then
    exit
fi
cp ./spl/u-boot-spl.bin ../imx-mkimage/iMX8M
cp ./u-boot-nodtb.bin ../imx-mkimage/iMX8M
cp ./tools/mkimage ../imx-mkimage/iMX8M/mkimage_uboot
cp ./arch/arm/dts/$DTB ../imx-mkimage/iMX8M/

cd ../imx-mkimage
make SOC=iMX8MM flash_ddr3l_val_no_hdmi
dd if=./iMX8M/flash.bin of=./iMX8M/u-boot.nb0 bs=1K skip=351
cp iMX8M/flash.bin ../flash_"$arch"_"$fert".bin

echo "Done"
