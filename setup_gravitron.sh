#!/usr/bin/env bash

# Update and install dependencies
sudo yum update
sudo yum install python glibc-devel ncurses-compat-libs.aarch64 environment-modules

# Download ARM Allinea Studio 20.3.1
wget -q -O- "https://developer.arm.com/-/media/Files/downloads/hpc/arm-allinea-studio/20-3-1/RHEL7/arm-compiler-for-linux_20.3_RHEL-7_aarch64.tar?revision=487ae64b-f895-4db4-acfd-b18b7b9e96b1" | tar x && sudo ./arm-compiler-for-linux_20.3_RHEL-7/arm-compiler-for-linux_20.3_RHEL-7.sh --accept
#wget -q -O- "https://developer.arm.com/-/media/Files/downloads/hpc/arm-allinea-studio/20-3-1/RHEL7/arm-forge-20.1.3-Redhat-7.6-aarch64.tar?revision=9174c084-4096-406e-9af3-a4ad7228631f" | tar x
#wget -q -O- "https://developer.arm.com/-/media/Files/downloads/hpc/arm-allinea-studio/20-3-1/RHEL7/arm-licenceserver-20.1.3-Redhat-7.6-aarch64.tar?revision=068295a0-e2aa-4c75-be05-21be898caac8" | tar x

sudo mkdir /opt/arm/licenses
sudo cp License.bin /opt/arm/licenses/

echo "export MODULEPATH=$MODULEPATH:/opt/arm/modulefiles/" >> .bashrc
echo "module load Generic-AArch64/RHEL/7/arm-linux-compiler/20.3" >> .bashrc
