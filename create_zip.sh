#!/bin/bash

# Check if an argument is provided
if [ -z "$1" ]; then
    echo "Error: Please provide an input name."
    echo "Usage: ./create_zip.sh <input_name>"
    exit 1
fi

ZIP_FILENAME="entrega_$1.zip"

echo "Cleaning up..."
make clean

echo "Compiling for Windows..."
make windows_fast -j16

echo "Copying Windows build to root temporarily so it zips without the 'build/windows/' prefix..."
cp build/windows/programa_compilado_windows64.zip ./programa_compilado_windows64.zip

echo "Zipping project files into $ZIP_FILENAME..."
zip -r "$ZIP_FILENAME" Makefile README.md LICENSE src/ libs/ imgui/ programa_compilado_windows64.zip

echo "Cleaning up temporary files..."
rm ./programa_compilado_windows64.zip

echo "Successfully created $ZIP_FILENAME!"
