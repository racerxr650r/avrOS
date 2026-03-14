# avrOS Application Makefile
#
# based on the makefile written by michael cousins (http://github.com/mcous)

# targets:
#   all:        compiles the source code
#   test:       tests the isp connection to the mcu
#   terminal:   opens an avrdude terminal connected to the MCU
#   flash:      writes compiled elf file to the mcu's flash memory
#   fuses:      writes the fuse bytes to the MCU
#   lock_bits:  writes the lock_bits register
#   complexity: calculate and display the complexity of the complete source 
#               code. See GNU Complexity
#   disasm:     disassembles the code for debugging
#   docs:       generates doxygen API documentation and PDF versions of
#               markdown files in build/docs directory
#   clean:      removes all .hex, .elf, and .o files in the source code and
#               library directories

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
CFLAGS    = -Wall -Wno-unused-function -Og -g2 -DDEBUG -mmcu=$(MCU) -B $(DFP)/gcc/dev/$(MCU) -funsigned-char -funsigned-bitfields -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -std=gnu99 -MMD -I$(DFP)/include -I. $(INCLUDE)
# any aditional flags for c++
CPPFLAGS =
# linker flags
LINKFLAGS = -Wl,-Map="$(OutputFileName).map" -Wl,--cref -Wl,--start-group -Wl,-lm -Wl,--end-group -Wl,--gc-sections -mmcu=avr128da28 -B $(DFP)/gcc/dev/$(MCU) -Wl,-T avrOS.x 
# name of new project directory
NAME = project

# executables
#AVRDUDE = /usr/local/bin/avrdude -c $(PRG) -p $(MCU)
AVRDUDE = /usr/bin/avrdude -c $(PRG) -p $(MCU)
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
all: build docs $(BUILD_DIR)$(PRJ).elf $(BUILD_DIR)cppcheck_report.txt $(BUILD_DIR)complexity_report.txt

# Create the build directory
build:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)cppcheck

# Generate the intel hex binary image
hex: $(BUILD_DIR)$(PRJ).hex

# generate html documentation
docs: build
	mkdir -p $(BUILD_DIR)docs
	cd ../../doc && doxygen Doxyfile
	mv ../../doc/api $(BUILD_DIR)docs/
	cp ../../doc/readme.html $(BUILD_DIR)docs/
	cp -r ../../doc/images $(BUILD_DIR)docs/
	pandoc -s --resource-path=../../doc ../../doc/MANUAL.md -o $(BUILD_DIR)docs/MANUAL.html
	pandoc -s --resource-path=../../doc ../../doc/SDD.md -o $(BUILD_DIR)docs/SDD.html
	pandoc -s --resource-path=../../doc ../../doc/PI4_Dev_Station.md -o $(BUILD_DIR)docs/PI4_Dev_Station.html

# test programmer connectivity
test:
	$(AVRDUDE) -v

# display AVR configuration
terminal:
	$(AVRDUDE) -v -t

# flash program to mcu
flash: all
	$(AVRDUDE) -U flash:w:$(BUILD_DIR)$(PRJ).elf:e

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
	cp avrOSConfig.h ../$(NAME)
	cp main.c ../$(NAME)
	cp makefile ../$(NAME)

# remove compiled files
clean:
	rm -rf $(BUILD_DIR)
#	rm -f BUILD_DIR/*.hex BUILD_DIR/*.elf BUILD_DIR/*.d BUILD_DIR/*.o
#	$(foreach dir, $(EXT), rm -f $(dir)/*.o;)
#	$(foreach dir, $(EXT), rm -f $(dir)/*.d;)

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
