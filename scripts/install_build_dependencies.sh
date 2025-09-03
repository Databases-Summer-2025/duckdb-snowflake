#!/bin/bash

# Install build dependencies required for vcpkg packages
# This script is called before vcpkg installation in CI environments

echo "Installing build dependencies for vcpkg packages..."

# Detect the operating system
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    # Linux
    if command -v apt-get &> /dev/null; then
        echo "Installing dependencies on Debian/Ubuntu..."
        sudo apt-get update -y -qq
        sudo apt-get install -y -qq flex bison
    elif command -v yum &> /dev/null; then
        echo "Installing dependencies on RHEL/CentOS..."
        sudo yum install -y flex bison
    elif command -v dnf &> /dev/null; then
        echo "Installing dependencies on Fedora..."
        sudo dnf install -y flex bison
    elif command -v apk &> /dev/null; then
        echo "Installing dependencies on Alpine..."
        apk add --no-cache flex bison
    else
        echo "Warning: Could not detect package manager"
    fi
elif [[ "$OSTYPE" == "darwin"* ]]; then
    # macOS
    echo "Installing dependencies on macOS..."
    if ! command -v brew &> /dev/null; then
        echo "Homebrew not found, skipping..."
    else
        brew install flex bison || true
        # Add bison to PATH (macOS often has an older version)
        echo "/opt/homebrew/opt/bison/bin" >> $GITHUB_PATH
        echo "/usr/local/opt/bison/bin" >> $GITHUB_PATH
    fi
elif [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "win32" ]]; then
    # Windows
    echo "Windows detected - flex/bison not required for MSVC builds"
else
    echo "Unknown OS: $OSTYPE"
fi

echo "Build dependencies installation completed."