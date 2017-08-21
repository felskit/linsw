#!/bin/sh

# Check input parameters
if [ $# -ne 1 ]
then
	echo "Usage: $0 <path to Buildroot root directory>"
	exit 1
fi

BR2_PATH=$1

if [ -e ${BR2_PATH}"/.config" ]
then
	echo "creating a backup of old Buildroot configuration in .config.old"
	cp ${BR2_PATH}"/.config" ${BR2_PATH}"/.config.old" || exit $?
fi

echo "applying default Raspberry Pi configuration"
make -C ${BR2_PATH} raspberrypi_defconfig || exit $?

echo "creating a directory for source files"
mkdir ${BR2_PATH}"/../felskit-lab3-user"

echo "copying overlay directories"
cp -R overlay ${BR2_PATH}"/../felskit-lab3-user" || exit $?

echo "copying python-magic package files"
cp -R package ${BR2_PATH} || exit $?

echo "copying patches"
cp .config.patch ${BR2_PATH}"/../felskit-lab3-user" || exit $?
cp Config.in.patch ${BR2_PATH}"/../felskit-lab3-user" || exit $?

cd ${BR2_PATH}
patch -N .config "../felskit-lab3-user/.config.patch"
patch -N package/Config.in "../felskit-lab3-user/Config.in.patch"

while true; do
	read -p "Do you want to compile the system image? [y/n] " yn
	case $yn in
		[Yy]* ) echo "compiling system image"; make; break;;
		[Nn]* ) exit $?;;
		* ) echo "Please answer y or n.";;
	esac
done
