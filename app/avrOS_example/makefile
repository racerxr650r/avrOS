# avrOS Application Makefile
#
# based on the makefile written by michael cousins (http://github.com/mcous)

# targets:
#   all:    compiles the source code
#   test:   tests the isp connection to the mcu
#   flash:  writes compiled hex file to the mcu's flash memory
#   fuse:   writes the fuse bytes to the MCU
#   disasm: disassembles the code for debugging
#   clean:  removes all .hex, .elf, and .o files in the source code and library directories

# parameters (change this stuff accordingly)
# project name
PRJ = main
# avr mcu
MCU = avr128da28
# mcu clock frequency
CLK = 24000000
# avr programmer (and port if necessary)
# e.g. PRG = usbtiny -or- PRG = arduino -P /dev/tty.usbmodem411
# The current programmer if the Atmel ICE w/UPDI interface
PRG = atmelice_updi
# fuse values for avr: low, high, and extended
# these values are from an Arduino Uno (ATMega328P)
# see http://www.engbedded.com/fusecalc/ for other MCUs and options
LFU = 0xFF
HFU = 0xDE
EFU = 0x05
# program source files (not including external libraries)
SRC = $(PRJ).c
# Device Family Pack directory
DFP = /usr/lib/gcc/avr/5.4.0/Atmel.AVR-Dx_DFP.2.2.253
# where to look for external libraries (consisting of .c/.cpp files and .h files)
# e.g. EXT = ../../EyeToSee ../../YouSART
EXT = ../.. ../../sys ../../drv ../../srv
# Build directory
BUILD_DIR = ./build/


#################################################################################################
# \/ stuff nobody needs to worry about until such time that worrying about it is appropriate \/ #
#################################################################################################

# include path
INCLUDE := $(foreach dir, $(EXT), -I$(dir))
# c flags
CFLAGS    = -Wall -Og -g2 -DDEBUG -mmcu=$(MCU) -B $(DFP)/gcc/dev/$(MCU) -funsigned-char -funsigned-bitfields -ffunction-sections -fdata-sections -fpack-struct -fshort-enums -std=gnu99 -MMD -I$(DFP)/include -I. $(INCLUDE)
# any aditional flags for c++
CPPFLAGS =
# linker flags
LINKFLAGS = -Wl,-Map="$(OutputFileName).map" -Wl,--start-group -Wl,-lm -Wl,--end-group -Wl,--gc-sections -mmcu=avr128da28 -B $(DFP)/gcc/dev/$(MCU) -Wl,-T avrOS.x 

# executables
AVRDUDE = /usr/local/bin/avrdude -c $(PRG) -p $(MCU)
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE    = avr-size --format=avr -B $(DFP)/gcc/dev/$(MCU) --mcu=$(MCU)
CC      = avr-gcc

# generate list of objects
VPATH  = $(EXT)
CFILES = $(filter %.c, $(SRC))
EXTC   = $(foreach dir, $(EXT), $(wildcard $(dir)/*.c))
SRCS   = $(CFILES) $(EXTC)
OBJ    = $(addprefix $(BUILD_DIR),$(notdir $(CFILES:%.c=%.o)) $(notdir $(EXTC:%.c=%.o)))
DEP    = $(OBJ:%.o=%.d)

# user targets
# compile all files
all: build $(BUILD_DIR)$(PRJ).hex 

# Create the build directory
build:
	mkdir -p $(BUILD_DIR)

# test programmer connectivity
test:
	$(AVRDUDE) -v

# flash program to mcu
flash: all
	$(AVRDUDE) -U flash:w:$(BUILD_DIR)$(PRJ).hex:i

# write fuses to mcu
fuse:
	$(AVRDUDE) -U lfuse:w:$(LFU):m -U hfuse:w:$(HFU):m -U efuse:w:$(EFU):m

# generate disassembly files for debugging
disasm: $(PRJ).elf
	$(OBJDUMP) -d $(BUILD_DIR)$(PRJ).elf

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
.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

# objects from c++ files
$(BUILD_DIR)%.o : %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# elf file
$(BUILD_DIR)$(PRJ).elf: $(OBJ)
	$(CC) $(LINKFLAGS) -o $(BUILD_DIR)$(PRJ).elf $(OBJ)

# hex file
$(BUILD_DIR)$(PRJ).hex: $(BUILD_DIR)$(PRJ).elf
	rm -f $(BUILD_DIR)$(PRJ).hex
	$(OBJCOPY) -O ihex -R .eeprom -R .fuse -R .lock -R .signature -R .user_signatures $(BUILD_DIR)$(PRJ).elf $(BUILD_DIR)$(PRJ).hex
