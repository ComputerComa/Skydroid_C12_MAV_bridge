# Debian development packages

This project currently builds with the standard GNU C++ toolchain, CMake, and
Ninja. The full MAVLink source repository is included as a Git submodule. CMake
uses its pinned pymavlink generator to build the C headers from XML definitions.

These instructions apply to Debian 12, Debian WSL2, and Raspberry Pi OS
Bookworm.

## Required packages

Update the package index and install the build tools:

```bash
sudo apt update
sudo apt install --yes \
    build-essential \
    ca-certificates \
    cmake \
    git \
    ninja-build \
    python3 \
    python3-lxml
```

The packages provide:

- `build-essential`: GCC, G++, the standard C/C++ libraries, and basic build
  tools.
- `ca-certificates`: trusted certificates needed for HTTPS Git operations.
- `cmake`: generates the project build files and registers the tests.
- `git`: obtains the repository and its MAVLink submodule.
- `ninja-build`: performs fast incremental builds from CMake's Ninja files.
- `python3`: runs MAVLink's C-header and Wireshark Lua generators.
- `python3-lxml`: validates MAVLink XML definitions against their schema during
  generation.

The project requires CMake 3.25 or newer and a compiler with C++20 support.

## Optional development tools

These packages are useful but are not required to build or run the project:

```bash
sudo apt install --yes \
    clang-format \
    gdb
```

- `clang-format` formats C and C++ source files.
- `gdb` debugs the executable and examines crashes.

### Reccomended install command

```bash
sudo apt install -y \
  build-essential \
  gcc \
  g++ \
  clang \
  clang-format \
  clang-tidy \
  cppcheck \
  cmake \
  ninja-build \
  pkg-config \
  git \
  gdb \
  gdbserver \
  openssh-client \
  rsync \
  python3 \
  python3-pip \
  libsystemd-dev \
  libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev
```

## Prepare the source tree

After cloning the repository, initialize the full MAVLink repository and its
nested pymavlink generator submodule:

```bash
git submodule update --init --recursive
```

## Build and verify

From the repository root:

```bash
cmake -S . -B build/debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
ctest --test-dir build/debug --output-on-failure
./build/debug/c12-bridge
```

CMake generates the `ardupilotmega` MAVLink 2 C headers under
`build/debug/generated/mavlink` as part of a normal build.

## Wireshark Lua dissector

Generate a Lua dissector from the same pinned MAVLink definitions:

```bash
cmake --build build/debug --target mavlink-wireshark
```

The generated file is:

```text
build/debug/generated/wireshark/mavlink.lua
```

The build does not install the dissector automatically because Wireshark plugin
locations vary by platform and version.

No C12 camera, Pixhawk, serial connection, or network access is required for
the current automated test.

## Raspberry Pi serial access

Serial access is configured through Linux permissions rather than an
additional package. When the serial transport is added, the service user will
normally need membership in the group that owns the selected serial device,
commonly `dialout` on Debian:

```bash
sudo usermod --append --groups dialout "$USER"
```

Log out and back in after changing group membership. Confirm the actual device
owner and group before applying this change:

```bash
ls -l /dev/serial0
```

Do not run the bridge as root merely to bypass serial-device permissions.

Video, RTSP, OpenCV, and C12-specific dependencies will be documented only when
those features are implemented.
