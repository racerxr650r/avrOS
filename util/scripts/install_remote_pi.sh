#! /bin/bash
# This script installs utilities and updates configurations to make a
# raspberry pi more useful in remote development mode
#
# Note: This script assumes that the $AVROSHOME environment variable has been set
#
# Note: This script replaces the Bash shell configuration script
#
if [ -z "$AVROSHOME" ]; then
    echo "Set AVROSHOME environment variable before running this script"
    echo "Goto the avrOS root directory and the run this command: export AVROSHOME=$(pwd)"
else
    sudo apt update
    sudo apt full-upgrade -y
    # Install the command line tools
    $AVROSHOME/util/scripts/install_cli_tools.sh

    # Install Bottom (btm) system status tool
    echo "Installing Bottom (btm) system status tool"
    wget https://github.com/ClementTsang/bottom/releases/download/0.9.6/bottom_0.9.6_arm64.deb
    sudo apt install -y ./bottom_0.9.6_arm64.deb
    rm bottom_0.9.6_arm64.deb

    # Copy the config files for bash, tio, and tmux to user home
    echo "Copying Bash, Tio, and Tmux configs to user home"
    cp --backup=simple -f $AVROSHOME/util/scripts/config/.bashrc ~
    cp --backup=simple -f $AVROSHOME/util/scripts/config/.tioconfig ~
    cp --backup=simple -f $AVROSHOME/util/scripts/config/.tmux.conf ~

    # Update /boot/config.txt to support serial console and uarts 2, 3, and 4
    echo "Modifying /boot/config.txt to support serial console and uarts 2, 3, and 4"

#    echo "[all]" | sudo tee -a /boot/config.txt
    grep -qxF 'alias complex' /boot/config.txt || printf "alias complex=\'tree -f -i -n -P *.c | grep .c | complexity -h -c --threshold=1\'\n\r" | sudo tee -a /boot/config.txt
    grep -qxF 'enable_uart=1' /boot/config.txt || echo 'enable_uart=1' | sudo tee -a /boot/config.txt
    grep -qxF 'dtoverlay=uart2' /boot/config.txt || echo 'dtoverlay=uart2' | sudo tee -a /boot/config.txt
    grep -qxF 'dtoverlay=uart3' /boot/config.txt || echo 'dtoverlay=uart3' | sudo tee -a /boot/config.txt
    grep -qxF 'dtoverlay=uart4' /boot/config.txt || echo 'dtoverlay=uart4' | sudo tee -a /boot/config.txt
fi
