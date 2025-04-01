#!/bin/bash
# compile.sh - Script to compile the GTK browser application

# Check if pkg-config is installed
if ! command -v pkg-config &> /dev/null; then
    echo "Error: pkg-config is not installed. Please install pkg-config and try again."
    exit 1
fi

# Check if GTK+ 3 development files are installed
if ! pkg-config --exists gtk+-3.0; then
    echo "Error: GTK+ 3 development files are missing. Install libgtk-3-dev."
    exit 1
fi

# Check if WebKit2GTK 4.1 development files are installed
if ! pkg-config --exists webkit2gtk-4.1; then
    echo "Error: WebKit2GTK 4.1 development files are missing. Install libwebkit2gtk-4.1-dev."
    exit 1
fi

# Compile the code using gcc and pkg-config for flags and libraries
gcc main.c -o gtkbrowser $(pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1)

# Check if compilation was successful
if [ $? -eq 0 ]; then
    echo "Compilation successful. Run './gtkbrowser' to launch the browser."
else
    echo "Compilation failed. Please check the error messages above."
fi

