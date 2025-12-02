#!/usr/bin/env bash
# Simplified pack generation script without packchk validation
# This creates a basic .pack file from the PDSC

set -e

PDSC_FILE="fool_cat.fc_embed.pdsc"
OUTPUT_DIR="output"
BUILD_DIR="build"

echo "=========================================="
echo "Simplified CMSIS Pack Generation"
echo "=========================================="
echo ""

# Check and install zip if needed
if ! command -v zip &> /dev/null; then
    echo "zip not found, installing..."
    if command -v apt-get &> /dev/null; then
        sudo apt-get update && sudo apt-get install -y zip
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y zip
    elif command -v yum &> /dev/null; then
        sudo yum install -y zip
    else
        echo "Error: Cannot install zip automatically. Please install it manually."
        exit 1
    fi
    echo "zip installed successfully."
    echo ""
fi

# Check if PDSC file exists
if [ ! -f "$PDSC_FILE" ]; then
    echo "Error: $PDSC_FILE not found!"
    exit 1
fi

# Extract version from PDSC
VERSION=$(grep -m1 '<release version=' "$PDSC_FILE" | sed 's/.*version="\([^"]*\)".*/\1/')
if [ -z "$VERSION" ]; then
    echo "Error: Could not extract version from PDSC file"
    exit 1
fi

VENDOR=$(grep -m1 '<vendor>' "$PDSC_FILE" | sed 's/.*<vendor>\(.*\)<\/vendor>.*/\1/')
NAME=$(grep -m1 '<name>' "$PDSC_FILE" | sed 's/.*<name>\(.*\)<\/name>.*/\1/')

PACK_NAME="${VENDOR}.${NAME}.${VERSION}.pack"

echo "Pack Information:"
echo "  Vendor:  $VENDOR"
echo "  Name:    $NAME"
echo "  Version: $VERSION"
echo "  Output:  $PACK_NAME"
echo ""

# Create directories
mkdir -p "$OUTPUT_DIR"
mkdir -p "$BUILD_DIR"

# Clean build directory
rm -rf "$BUILD_DIR"/*

# Copy files to build directory
echo "Copying files to build directory..."

# Copy PDSC file
cp "$PDSC_FILE" "$BUILD_DIR/"

# Copy LICENSE
if [ -f "LICENSE" ]; then
    cp "LICENSE" "$BUILD_DIR/"
fi

# Copy readme
if [ -f "readme.md" ]; then
    cp "readme.md" "$BUILD_DIR/"
fi

# Copy core directory
if [ -d "core" ]; then
    echo "Copying core directory..."
    cp -r "core" "$BUILD_DIR/"
fi

echo ""
echo "Creating pack archive..."

# Change to build directory and create zip
cd "$BUILD_DIR"
zip -r "../$OUTPUT_DIR/$PACK_NAME" . -q

cd ..

echo ""
echo "=========================================="
echo "Pack generation completed!"
echo "=========================================="
echo ""
echo "Output: $OUTPUT_DIR/$PACK_NAME"
echo ""

# Show file size
if [ -f "$OUTPUT_DIR/$PACK_NAME" ]; then
    SIZE=$(du -h "$OUTPUT_DIR/$PACK_NAME" | cut -f1)
    echo "Pack size: $SIZE"
fi

exit 0
