#!/bin/bash

# Download ADBC headers from official releases
ADBC_VERSION="apache-arrow-adbc-19"
HEADER_VERSION="1.7.0"

# Create directory for headers
HEADER_DIR="$(dirname "$0")/../src/include/arrow-adbc"
mkdir -p "$HEADER_DIR"

echo "Downloading ADBC headers..."

# Download the main adbc.h header
curl -L -o "$HEADER_DIR/adbc.h" \
    "https://raw.githubusercontent.com/apache/arrow-adbc/main/c/include/arrow-adbc/adbc.h"

# Download the driver manager header
curl -L -o "$HEADER_DIR/adbc_driver_manager.h" \
    "https://raw.githubusercontent.com/apache/arrow-adbc/main/c/include/arrow-adbc/adbc_driver_manager.h"

if [ -f "$HEADER_DIR/adbc.h" ] && [ -f "$HEADER_DIR/adbc_driver_manager.h" ]; then
    echo "ADBC headers downloaded successfully to: $HEADER_DIR"
else
    echo "Failed to download ADBC headers"
    exit 1
fi