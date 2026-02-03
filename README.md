# gwi

A Qt-based graphical user interface application for a Digital Microfluidics-based PCR test result detection and identification system. 

This project is part of the complete DMF-PCR results detection system, serves as the GUI codebase implemented on a Raspberry Pi 4 model B. The optical system employs a blue excitation light and a CMOS sensor, both of which are powered by the SBC.

## 1. Hardware Requirements

1. Main SBC    : Raspberry Pi 4 model B (4 GB model)
2. Sensor      : OV5647 5 MP CMOS sensor (Raspberry Pi Camera Module v1.3)
3. DMF control : Arduino Nano as the main controller, communicated with using hardware interrupt

## 2. Building Prerequisites

External libraries: WiringPi, OpenCV-mobile, LCCV, Qt6- plugins.

### 2.1 Cloning this repository

```sh
sudo apt install git
git clone https://github.com/arkandzprogaming/gwi.git
```

### 2.2 WiringPi

This project uses the [WiringPi](https://github.com/WiringPi/WiringPi.git) library to access the GPIO pins of the Raspberry Pi 4 model B.  
The [installation script](https://github.com/WiringPi/WiringPi?tab=readme-ov-file#from-source) provided by the official repository can be used to build the library from source, as shown below.

During the development of this project, WiringPi version **3.16** was built from source on an ARM-based Debian Bookworm system.

```sh
sudo apt install git
git clone https://github.com/WiringPi/WiringPi.git
cd WiringPi

./build debian
mv debian-template/wiringpi-3.16.deb .

sudo apt install ./wiringpi-3.16.deb
```

### 2.3 OpenCV Mobile

The source files can be downloaded directly according to the OS and system architecture being used [here](https://github.com/nihui/opencv-mobile.git). During development of this project, the aarch64 Bookworm version was used, [go to Environment Setup](#3-environment-setup) for more details.

After installation having downloaded the zipped source files, unzip them into the project directory (the same directory as the CMakelists.txt file). The latter file may need to be conditioned to include/link the specific opencv-mobile version being used.

### 2.4 LCCV

LCCV is a C++ Libcamera wrapper library for use with OpenCV. Following the instructions in its [official README](https://github.com/kbarni/LCCV?tab=readme-ov-file#building-and-installing), one ought to build and install the repository, preferably outside of this project directory. Make sure all prerequisites are thoroughly considered.

### 2.5 Qt6- plugins

All Qt6 libraries and plugins required by the CMakeLists.txt file need to be installed using APT. At the time of the development, Qt version 6.4.2 is used.

## 3. Environment Setup

### 3.1 Deployment environment

1. OS              : aarch64 Debian 12 (Bookworm) 
2. Kernel          : 6.12.62+rpt-ri-v8
3. Display backend : X11

### 3.2 User setup

Copy these lines into the tail of your `${HOME}/.bashrc` file.

```sh
export DISPLAY=:0.0
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/qt6/lib/
export XDG_RUNTIME_DIR=/run/user/1000
export RESOURCE_FOLDER_PATH=${HOME}/gwi/resources/
```

## Building

There are two modes of operations for the detection system: TIMER mode means that the sensor is toggled at a specific, hard-coded intervals (typically used for debugging), while ISR mode means that the system is ready to receive hardware interrupts sent by the DMF-PCR system.

```sh
mkdir build; cd build

# Building for TIMER operation
cmake ..; make -j2

# Building for ISR operation
cmake -DUSE_TIMER_MODE=OFF ..; make -j2
```

Run the executable while retaining the user-setup environment.

```sh
sudo -E ./appgwi
```
