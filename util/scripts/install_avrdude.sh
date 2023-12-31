#! /bin/bash
# This script installs the latest AVRDUDE application from the github repository
#

# Install required build tools and libs
sudo apt-get update
sudo apt-get install -y build-essential git cmake flex bison libelf-dev libusb-dev libhidapi-dev libftdi1-dev libreadline-dev

# Get the latest avrdude source, build it, and install it
git clone https://github.com/avrdudes/avrdude.git
cd avrdude
./build.sh
sudo cmake --build build_linux --target install
cd ..
rm -rf avrdude