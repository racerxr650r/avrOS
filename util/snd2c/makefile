# snd2c application Makefile

# Makefile Targets
#          all:	compiles the source code
#        clean: removes all .hex, .elf, and .o files in the source code and 
#              	library directories
#      install:	installs the application and documentation
#    uninstall:	uninstalls the application and documentation
#      prereqs: install the FFmpeg libraries

# Build Variables +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Project name
TARGET = snd2c
# Linux usr binaries directory
BINDIR =	/usr/local/bin
# Linux manual directory for the man command
MANDIR =	/usr/local/man/man1
# Build directory
BUILD_DIR = ./build
# Systemd services directory
SYSDDIR = /etc/systemd/system
# Udev rules directory
UDEVDIR = /etc/udev/rules.d
# C compiler command
CC =		gcc
# Build flags for c files
CFLAGS =	-O -I/usr/local/include -pedantic -Wall -Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wredundant-decls -Wno-long-long
# Linker Flags
LFLAGS =	-lavcodec -lavformat -lswresample -lavutil

# Build Targets +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# Build all target files
all:		build $(TARGET)
# Create the build directory
build:
	mkdir -p $(BUILD_DIR)
# Build the app from the .c source
$(TARGET):		$(TARGET).c
	$(CC) $(CFLAGS) $(TARGET).c -o $(BUILD_DIR)/$(TARGET) $(LFLAGS)
# Install the application
install:	all
	sudo cp $(BUILD_DIR)/$(TARGET) $(BINDIR)
	sudo cp $(TARGET).1 $(MANDIR)
# Uninstall the application
uninstall:
	sudo rm -f $(BINDIR)/$(TARGET)
	sudo rm -f $(MANDIR)/$(TARGET).1
# Install prereqeuisites
prereqs:
	sudo apt update
	sudo apt install libavcodec-dev libavformat-dev libswresample-dev
# Clean up all generated files
clean:
	rm -rf $(BUILD_DIR)
