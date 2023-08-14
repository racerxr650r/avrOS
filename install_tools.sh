#! /bin/bash

sudo apt-get update

# Install make, git, gcc-avr, gcc tools, and unzip
sudo apt-get install -y make binutils gcc-avr gdb-avr avr-libc flex byacc bison cmake unzip 

# Install command line app tools
sudo apt-get install -y minicom screens micro 

# Install visual tools
sudo apt-get install -y gtkterm geany geany-plugins git-cola meld code

# Get geany color scheme(s)
wget -P ~/.config/geany/colorschemes https://raw.github.com/geany/geany-themes/master/colorschemes/delt-dark.conf
wget -P ~/.config/geany/colorschemes https://raw.github.com/geany/geany-themes/master/colorschemes/dark-colors.conf
wget -P ~/.config/geany/colorschemes https://raw.github.com/geany/geany-themes/master/colorschemes/himbeere.conf
wget -P ~/.config/geany/colorschemes https://raw.github.com/geany/geany-themes/master/colorschemes/mc.conf

# Get the AVR Dx DFP file for the AVR DA libs and header files
wget http://packs.download.atmel.com/Atmel.AVR-Dx_DFP.2.2.253.atpack

# Unzip the DFP package, copy it to the compiler directory, and clean up
mkdir ./Atmel.AVR-Dx_DFP.2.2.253
unzip -d Atmel.AVR-Dx_DFP.2.2.253/ Atmel.AVR-Dx_DFP.2.2.253.atpack
sudo cp -R Atmel.AVR-Dx_DFP.2.2.253/ /usr/lib/gcc/avr/5.4.0
rm -rf Atmel.AVR-Dx_DFP.2.2.253
rm Atmel.AVR-Dx_DFP.2.2.253.atpack

# Get the latest avrdude source, build it, and install it
./install_avrdude.sh
rm -rf ../avrdude

# Goto the avrOS_example directory and build the application image (.hex)
pushd .
cd ./app/avrOS_example
make
popd
