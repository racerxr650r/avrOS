#! /bin/bash
# This script installs the relevant command line tools for avrOS
#
# Note: This script assumes the $AVROSHOME environment variable has been set
#

if [ -z "${AVROSHOME}"]; then
    echo "Set AVROSHOME environment variable before running this script"
    echo "Goto the avrOS root directory and the run this command: export AVROSHOME=$(pwd)"
else
    # Update apt-get to the latest in the repositories
    sudo apt-get update

    # Install make, git, gcc-avr, gcc tools, and unzip
    sudo apt-get install -y make binutils gcc-avr gdb-avr avr-libc flex byacc bison cmake unzip avrdude

    # Install command line app tools
    sudo apt-get install -y minicom tio screens tmux micro tree devmon xterm

    # Get the AVR Dx DFP file for the AVR DA libs and header files
    wget http://packs.download.atmel.com/Atmel.AVR-Dx_DFP.2.2.253.atpack
    # Unzip the DFP package, copy it to the compiler directory, and clean up
    mkdir ./Atmel.AVR-Dx_DFP.2.2.253
    unzip -d Atmel.AVR-Dx_DFP.2.2.253/ Atmel.AVR-Dx_DFP.2.2.253.atpack
    sudo cp -R Atmel.AVR-Dx_DFP.2.2.253/ /usr/lib/gcc/avr/5.4.0
    rm -rf Atmel.AVR-Dx_DFP.2.2.253
    rm Atmel.AVR-Dx_DFP.2.2.253.atpack

    # Get the latest avrdude source, build it, and install it
    #./install_avrdude.sh
    #rm -rf ../avrdude

    # Goto the avrOS_example directory and build the application image (.hex)
    pushd .
    cd $AVROSHOME/app/avrOS_example
    make
    popd

    # Setup git username and email address
    $AVROSHOME/util/scripts/setup_git.sh
fi