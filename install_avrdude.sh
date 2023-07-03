#! /bin/bash

# Install required build tools and libs
sudo apt-get update
sudo apt-get install -y build-essential git cmake flex bison libelf-dev libusb-dev libhidapi-dev libftdi1-dev libreadline-dev

# Goto the "projects" directory
pushd .
cd ..

# Get the latest avrdude source, build it, and install it
git clone https://github.com/avrdudes/avrdude.git
cd avrdude
./build.sh
sudo cmake --build build_linux --target install
cd ..

# Return to original directory
popd
