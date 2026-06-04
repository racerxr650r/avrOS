# avrOS Application Makefile
#
# This makefile is designed to compile and manage an application built on top
# of the avrOS real-time operating system for AVR microcontrollers. It provides
# targets for building the application, testing the programmer connection,
# flashing the compiled code to the microcontroller, and generating
# documentation, among other tasks.
#
# Targets:
#   all         Compile all source files and generate ELF, static analysis,
#               and complexity reports (default target)
#   build       Create the build output directory structure
#   hex         Generate the Intel HEX binary image from the ELF
#   flash       Write the compiled ELF to the MCU's flash memory
#   fuses       Write the fuse bytes to the MCU
#   lock_bits   Write the lock bits register on the MCU
#   test        Test the ISP/UPDI programmer connection to the MCU
#   terminal    Open an avrdude terminal session connected to the MCU
#   docs        Generate Doxygen API documentation and HTML versions of
#               the markdown files; output goes to build/docs/
#   complexity  Calculate and display cyclomatic complexity of the source
#               code (requires GNU Complexity)
#   analyze     Run cppcheck static analysis and display results
#   sloc        Run sloccount SLOC estimation with cost/schedule projection
#   disasm      Disassemble the ELF file for debugging
#   version     Update AVROS_VERSION in avrOSConfig.h with the version string\n#               from the root VERSION file
#   project     Create a new skeleton project directory (add "NAME=<dir>" at the
#               command line to specify the name; default is "project")
#   clean       Remove the entire build/ output directory
#   prereqs     For debian-based systems (MX Linux, Raspberry Pi OS, Ubuntu,
#               etc.), install the required packages
#   setup_pi    Set up a Raspberry Pi for AVR development. Enable serial
#				console and UARTs.
#   help        Display this help message

# project version (read from root VERSION file)
VERSION := $(shell cat ../../VERSION)

# parameters (change this stuff accordingly)
# project name
PRJ = main

# avr mcu
MCU = avr128da28
# mcu clock frequency
CLK = 24000000

# avr programmer (and port if necessary)
# e.g. PRG = atmelice_updi -or- PRG = serialupdi -P /dev/ttyUSB0
# The current programmer if the Atmel ICE w/UPDI interface
PRG = serialupdi -P /dev/ttyAMA2
#PRG = serialupdi -P /dev/ttyUSB2
PORT = /dev/ttyAMA2

# program source files (not including external libraries)
SRC = $(PRJ).c
# Device Family Pack directory (update this if you update the DFP)
DFP = /usr/lib/gcc/avr/5.4.0/Atmel.AVR-Dx_DFP.2.4.286
# where to look for external libraries (consisting of .c/.cpp files and .h files)
# e.g. EXT = ../../EyeToSee ../../YouSART
EXT = ../.. ../../sys ../../drv ../../srv
# Build directory
BUILD_DIR = ./build/
# include path
INCLUDE := $(foreach dir, $(EXT), -I$(dir))

# c flags
CFLAGS    = -Wall -Wno-unused-function -O0 -g3 -fno-lto -fno-inline -fno-ipa-icf -fno-ipa-cp-clone -fvar-tracking-assignments -DDEBUG -mmcu=$(MCU) -B $(DFP)/gcc/dev/$(MCU) -funsigned-char -funsigned-bitfields -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -std=gnu99 -MMD -I$(DFP)/include -I. $(INCLUDE)
# any aditional flags for c++
CPPFLAGS =
# linker flags
LINKFLAGS = -Wl,-Map="$(OutputFileName).map" -Wl,--cref -Wl,--start-group -Wl,-lm -Wl,--end-group -Wl,--gc-sections -mmcu=avr128da28 -B $(DFP)/gcc/dev/$(MCU) -Wl,-T avrOS-sections.x 
# name of new project directory
NAME = project

# executables
#AVRDUDE = /usr/local/bin/avrdude -c $(PRG) -p $(MCU)
AVRDUDE = /usr/bin/avrdude -c $(PRG) -p $(MCU)
PROGAMMER = avrOSdb --prog
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE    = avr-size --format=avr -B $(DFP)/gcc/dev/$(MCU) --mcu=$(MCU)
CC      = avr-gcc

# annual salary used to calculate cost for the SLOC estimation (Source indeed.com)
SALARY = 119623
# Effort factor used to calculate effort in the SLOC estimation (Semidetached)
EFFORT_FACTOR = 3
# Effort exponent used to calculate effort in the SLOC estimation (Semidetached)
EFFORT_EXPONENT = 1.12
# Schedule factor used to calculate schedule in the SLOC estimation (Semidetached)
SCHEDULE_FACTOR = 2.5
# Schedule exponent used to calculate schedule in the SLOC estimation (Semidetached)
SCHEDULE_EXPONENT = 0.35

# generate list of objects
VPATH  = $(EXT)
CFILES = $(filter %.c, $(SRC))
EXTC   = $(foreach dir, $(EXT), $(wildcard $(dir)/*.c))
SRCS   = $(CFILES) $(EXTC)
OBJ    = $(addprefix $(BUILD_DIR),$(notdir $(CFILES:%.c=%.o)) $(notdir $(EXTC:%.c=%.o)))
DEP    = $(OBJ:%.o=%.d)

# user targets
# compile all files
all: version build $(BUILD_DIR)$(PRJ).elf

# Create the build directory
build:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)cppcheck
	mkdir -p $(BUILD_DIR)docs

# Generate the intel hex binary image
hex: $(BUILD_DIR)$(PRJ).hex

# generate html documentation
docs: build
#	pandoc -s --resource-path=../../doc --css github-dark.css ../../doc/Main_Page.md -o $(BUILD_DIR)docs/Main_Page.html
	pandoc --resource-path=../../doc ../../doc/Main_Page.md -o ../../doc/Main_Page.html
	pandoc -s --resource-path=../../doc --css github-dark.css ../../doc/MANUAL.md -o $(BUILD_DIR)docs/MANUAL.html
	pandoc -s --resource-path=../../doc --css github-dark.css ../../doc/SDD.md -o $(BUILD_DIR)docs/SDD.html
	cd ../../doc && VERSION=$(VERSION) doxygen Doxyfile
	rm -f ../../doc/Main_Page.html
	rm -rf $(BUILD_DIR)docs/api
	mv ../../doc/api $(BUILD_DIR)docs/
	cp ../../doc/index.html $(BUILD_DIR)docs/
	cp -r ../../doc/images $(BUILD_DIR)docs/
	cp ../../doc/github-dark.css $(BUILD_DIR)docs/
	@echo "Docs: file://$(shell hostname)$(abspath $(BUILD_DIR)docs/index.html)"

# update AVROS_VERSION in avrOSConfig.h with the version string from the root VERSION file
version:
	sed -i 's/\(#define AVROS_VERSION\s*\)"[^"]*"/\1"$(VERSION)"/' avrOSConfig.h
	@echo "AVROS_VERSION updated to $(VERSION)"

# test programmer connectivity
test:
	$(AVRDUDE) -v

# display AVR configuration
terminal:
	$(AVRDUDE) -v -t

# flash program to mcu
flash: all
	$(PROGAMMER) $(PORT) $(BUILD_DIR)$(PRJ).elf

# write fuses to mcu
fuses: all
	$(AVRDUDE) -U fuses:w:$(BUILD_DIR)$(PRJ).elf:e

# write lock bits to mcu
lock_bits: all
	$(AVRDUDE) -U lock:w:$(BUILD_DIR)$(PRJ).elf:e

# determine/display source complexity
complexity:
	complexity -H -h -c --threshold=0 $(SRCS)

# static code analysis
# --output-file=$(BUILD_DIR)cppcheck_report.txt 
analyze: build
	cppcheck -v --std=c99 --platform=avr8 --library=avr.cfg --max-ctu-depth=10 --cppcheck-build-dir=$(BUILD_DIR)cppcheck --language=c --inconclusive -I$(DFP)/include -I. $(INCLUDE) $(SRCS)

# SLOC Count
sloc: build
	sloccount --personcost $(SALARY) --effort $(EFFORT_FACTOR) $(EFFORT_EXPONENT) --schedule $(SCHEDULE_FACTOR) $(SCHEDULE_EXPONENT) --datadir ./build . $(EXT)

# generate disassembly files for debugging
disasm: $(PRJ).elf
	$(OBJDUMP) -d $(BUILD_DIR)$(PRJ).elf

# create new project directory
project:
	mkdir ../$(NAME)
	cp avrOS.x ../$(NAME)
	cp avrOS-sections.x ../$(NAME)
	cp avrOSConfig.h ../$(NAME)
	cp main.c ../$(NAME)
	cp makefile ../$(NAME)

# For debian-based systems (MX Linux, Raspberry Pi OS, Ubuntu, etc.), install
# the required packages
prereqs:
	echo "Installing prerequisite tools for avrOS development"
	sudo apt-get update
	sudo apt-get install -y --no-install-recommends \
		make binutils gcc-avr gdb-avr avr-libc flex byacc bison cmake unzip avrdude \
		sloccount complexity cppcheck doxygen graphviz pandoc
	# Get the AVR Dx DFP file for the AVR DA libs and header files
	echo "Installing the Microchip/Atmel Device Feature Pack for the AVR-Dx"
	wget http://packs.download.atmel.com/Atmel.AVR-Dx_DFP.2.4.286.atpack
	# Unzip the DFP package, copy it to the compiler directory, and clean up
	mkdir ./Atmel.AVR-Dx_DFP.2.4.286
	unzip -d Atmel.AVR-Dx_DFP.2.4.286/ Atmel.AVR-Dx_DFP.2.4.286.atpack
	sudo cp -R Atmel.AVR-Dx_DFP.2.4.286/ /usr/lib/gcc/avr/5.4.0
	rm -rf Atmel.AVR-Dx_DFP.2.4.286
	rm Atmel.AVR-Dx_DFP.2.4.286.atpack
	

# Set up Raspberry Pi for AVR development
setup_pi:
	echo "Modifying /boot/config.txt to support serial console and uarts 2, 3, and 4"
	grep -qxF 'alias complex' /boot/config.txt || printf "alias complex=\'tree -f -i -n -P *.c | grep .c | complexity -h -c --threshold=1\'\n\r" | sudo tee -a /boot/config.txt
	grep -qxF 'enable_uart=1' /boot/config.txt || echo 'enable_uart=1' | sudo tee -a /boot/config.txt
	grep -qxF 'dtoverlay=uart2' /boot/config.txt || echo 'dtoverlay=uart2' | sudo tee -a /boot/config.txt
	grep -qxF 'dtoverlay=uart3' /boot/config.txt || echo 'dtoverlay=uart3' | sudo tee -a /boot/config.txt
	grep -qxF 'dtoverlay=uart4' /boot/config.txt || echo 'dtoverlay=uart4' | sudo tee -a /boot/config.txt

# remove compiled files
clean:
	rm -rf $(BUILD_DIR)

# display help (strip leading '# ' from the file header comment block)
help:
	@awk '!/^#/{exit} {sub(/^# ?/,""); print}' $(MAKEFILE_LIST)

# Inlcude the dependency files
-include $(DEP)

# other targets
# objects from c files
#.c.o:
#	$(CC) $(CFLAGS) -c $< -o $@
#	cppcheck --enable=all $<

# objects from c files
$(BUILD_DIR)%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

# elf file
$(BUILD_DIR)$(PRJ).elf: $(OBJ)
	$(CC) $(LINKFLAGS) -o $(BUILD_DIR)$(PRJ).elf $(OBJ)

# hex file
$(BUILD_DIR)$(PRJ).hex: $(BUILD_DIR)$(PRJ).elf
	rm -f $(BUILD_DIR)$(PRJ).hex
	$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures $(BUILD_DIR)$(PRJ).elf $(BUILD_DIR)$(PRJ).hex

# Static analysis report 
$(BUILD_DIR)cppcheck_report.txt:
	cppcheck -v --std=c99 --platform=avr8 --library=avr.cfg --max-ctu-depth=10 --cppcheck-build-dir=$(BUILD_DIR)cppcheck --language=c --output-file=$(BUILD_DIR)cppcheck_report.txt -I$(DFP)/include -I. $(INCLUDE) $(SRCS)

# Complexity analysis report
$(BUILD_DIR)complexity_report.txt:
	complexity -H -h -c --threshold=0 $(SRCS) > $(BUILD_DIR)complexity_report.txt
