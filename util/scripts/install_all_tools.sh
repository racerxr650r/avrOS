#! /bin/bash
# This script installs both the CLI and GUI tools
#
# Note: This script assumes $AVROSHOME is set
#

if [ -z "$AVROSHOME" ]; then
    echo "Set AVROSHOME environment variable before running this script"
    echo "Goto the avrOS root directory and the run this command: export AVROSHOME=$(pwd)"
else
    # Install the command line tools
    ./install_cli_tools.sh
    # Install the graphical tools
    ./install_gui_tools.sh
fi