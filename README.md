# Flocking Project

---
This project could be used as a template for any C/C++ project.

Read the **[User Guide](doc/user_guide.md)** to learn more!
---

## Key features:

 - Cmake is used to as build-system generator
 - Build release builds
 - Gtest is the default unit-test framework
 - Debian install package


## Directory structure

```
.
├── app
│   └── flock
│       └── resources
├── cmake
├── debian
└── scripts

```

## To Compile project

```

  ./scripts/build.sh -dce

```

## To install raylib

```

git clone https://github.com/raysan5/raylib.git raylib
cd raylib
mkdir build && cd build
cmake ..
make
sudo make install
sudo ldconfig

```
