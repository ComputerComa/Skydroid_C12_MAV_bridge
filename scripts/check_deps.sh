#!/usr/bin/env bash

# Color tokens for high scannability
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m'

# Resolve paths relative to this script so the check works from any directory.
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "$SCRIPT_DIR/.." && pwd)"

echo "Verifying local C/C++ toolchain and library dependencies..."
FAILED=0

# Lists matching your custom environment setup
CLI_DEPS=(
    "gcc" "g++" "clang" "clang-format" "clang-tidy" 
    "cppcheck" "cmake" "ninja" "pkg-config" "git" 
    "gdb" "gdbserver" "ssh-add" "rsync"
)

LIB_DEPS=(
    "build-essential"
    "libsystemd-dev" 
    "libgstreamer1.0-dev" 
    "libgstreamer-plugins-base1.0-dev"
)

# 1. Audit System Binary Paths
for cmd in "${CLI_DEPS[@]}"; do
    if ! command -v "$cmd" &> /dev/null; then
        echo -e "${RED}[MISSING]${NC} Binary Tool: $cmd"
        FAILED=1
    else
        echo -e "${GREEN}[OK]${NC} Binary Tool: $cmd"
    fi
done

# 2. Audit Installed C Dev Libraries Natively via Dpkg
for lib in "${LIB_DEPS[@]}"; do
    if ! dpkg-query -W -f='${Status}' "$lib" 2>/dev/null | grep -q "ok installed"; then
        echo -e "${RED}[MISSING]${NC} Dev Library: $lib"
        FAILED=1
    else
        echo -e "${GREEN}[OK]${NC} Dev Library: $lib"
    fi
done

# 3. Structural Validation for MAVLink Submodule Headers
# This is the dialect header included by the bridge source code.
MAVLINK_HEADER_PATH="$REPO_ROOT/extern/mavlink/ardupilotmega/mavlink.h"
if [ ! -f "$MAVLINK_HEADER_PATH" ]; then
    echo -e "${YELLOW}[MISSING]${NC} Submodule: MAVLink headers missing at $MAVLINK_HEADER_PATH"
    FAILED=1
else
    echo -e "${GREEN}[OK]${NC} Submodule: MAVLink ArduPilotMega header detected"
fi

# 4. Optional Installation Prompt
if [ $FAILED -ne 0 ]; then
    echo -e "\n${YELLOW}⚠️ Missing dependencies or submodules found.${NC}"
    read -p "Would you like to fix the workspace environment now? (y/N): " choice
    case "$choice" in 
        [yY][eE][sS]|[yY]) 
            # Detect if missing dependencies require apt updates
            NEED_APT=0
            for cmd in "${CLI_DEPS[@]}"; do
                if ! command -v "$cmd" &> /dev/null; then NEED_APT=1; fi
            done
            for lib in "${LIB_DEPS[@]}"; do
                if ! dpkg-query -W -f='${Status}' "$lib" 2>/dev/null | grep -q "ok installed"; then NEED_APT=1; fi
            done

            if [ $NEED_APT -eq 1 ]; then
                echo "Updating apt package index..."
                sudo apt update
                echo "Installing workspace dependencies..."
                sudo apt install -y \
                    build-essential gcc g++ clang clang-format clang-tidy \
                    cppcheck cmake ninja-build pkg-config git gdb gdbserver \
                    openssh-client rsync python3 python3-pip libsystemd-dev \
                    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev
            fi

            # Fetch the submodule if the header used by this project is missing.
            if [ ! -f "$MAVLINK_HEADER_PATH" ]; then
                echo "Initializing Git submodules..."
                git -C "$REPO_ROOT" submodule update --init --recursive
            fi
            
            # Re-verify after installation attempt
            exec "$0"
            ;;
        *)
            echo -e "${RED}🛑 Environment validation skipped. Workspace setup incomplete.${NC}"
            exit 1
            ;;
    esac
else
    echo -e "\n${GREEN}🚀 Complete C/C++ workspace successfully verified!${NC}"
    exit 0
fi
