#! /bin/bash

# Goto the "projects" directory
pushd .
cd ..

# Install make, git, gcc-avr, gcc tools, and unzip
sudo apt install make binutils gcc-avr avr-libc flex byacc bison cmake libelf-dev libusb-dev libhidapi-dev libftdi1-dev libreadline-dev unzip

# Get the AVR Dx DFP file for the AVR DA libs and header files
wget http://packs.download.atmel.com/Atmel.AVR-Dx_DFP.2.2.253.atpack

# Unzip the DFP package, copy it to the compiler directory, and clean up
mkdir ./Atmel.AVR-Dx_DFP.2.2.253
unzip -d Atmel.AVR-Dx_DFP.2.2.253/ Atmel.AVR-Dx_DFP.2.2.253.atpack
sudo cp -R Atmel.AVR-Dx_DFP.2.2.253/ /usr/lib/gcc/avr/5.4.0
rm -rf Atmel.AVR-Dx_DFP.2.2.253
rm Atmel.AVR-Dx_DFP.2.2.253.atpack

# Get the latest avrdude source, build it, and install it
git clone https://github.com/avrdudes/avrdude.git
cd avrdude
./build.sh
sudo cmake --build build_linux --target install
cd ..

# Goto the avrOS_example directory and build the application image (.hex)
cd avrOS/app/avrOS_example
make

# Return to original directory
popd
