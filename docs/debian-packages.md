# Debian development packages

This project currently builds with the standard GNU C++ toolchain, CMake, and
Ninja. The MAVLink C library is header-only and is included as a Git submodule,
so it does not require a separate Debian package.

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
    ninja-build
```

The packages provide:

- `build-essential`: GCC, G++, the standard C/C++ libraries, and basic build
  tools.
- `ca-certificates`: trusted certificates needed for HTTPS Git operations.
- `cmake`: generates the project build files and registers the tests.
- `git`: obtains the repository and its MAVLink submodule.
- `ninja-build`: performs fast incremental builds from CMake's Ninja files.

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

After cloning the repository, initialize the official MAVLink headers:

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
