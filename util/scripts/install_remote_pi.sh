#! /bin/bash
# This script installs a fre utilities and updates configurations to make a
# raspberry pi more useful in remote development mode
#
# Note: This script 
sudo apt update
wget https://github.com/ClementTsang/bottom/releases/download/0.9.6/bottom_0.9.6_arm64.deb
sudo apt install -y ./bottom_0.9.6_arm64.deb
sduo apt install -y tio tmux xterm

cp --backup -f qconfig/.tioconfig ~
cp --backup -f config/.bashrc ~
cp --backup -f config/.tmux.conf ~
