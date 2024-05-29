# eXtra-fast Essential Video Decoder (XEVD)

[![Build](https://github.com/mpeg5/xevd/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/mpeg5/xevd/actions/workflows/build.yml)

The **eXtra-fast Essential Video Decoder** (XEVD) is an opensource and fast MPEG-5 EVC decoder.

**MPEG-5 Essential Video Coding** (EVC) is a video compression standard of ISO/IEC Moving Picture Experts Group (MPEG). The main goal of the EVC is to provide a significantly improved compression capability over existing video coding standards with timely publication of terms.
The EVC defines two profiles, including "**Baseline Profile**" and "**Main Profile**". The "Baseline profile" contains only technologies that are older than 20 years or otherwise freely available for use in the standard. In addition, the "Main profile" adds a small number of additional tools, each of which can be either cleanly disabled or switched to the corresponding baseline tool on an individual basis.

## How to build

### Linux (64-bit)
- Build Requirements
  - CMake 3.5 or later (download from [https://cmake.org/](https://cmake.org/))
  - GCC 5.4.0 or later

- Build Instructions
  ```
  $mkdir build
  $cd build
  $cmake ..
  $make
  ```
  - Output Location
    - Executable application (xevd_app) can be found under build/bin/.
    - Library files (libxevd.so and libxevd.a) can be found under build/lib/.

### Windows (64-bit)
- Build Requirements
  - CMake 3.5 or later (download from [https://cmake.org/](https://cmake.org/))
  - MinGW-64 or Microsoft Visual Studio

- Build Instructions
  - MinGW-64
    ```
    $mkdir build
    $cd build
    $cmake .. -G "MinGW Makefiles"
    $make
    ```
  - Microsoft Visual Studio
    ```
    $mkdir build
    $cd build
    $cmake .. -G "Visual Studio 15 2017 Win64"
    $make
    ```
    You can change '-G' option with proper version of Visual Studio.

### ARM (64-bit)
- Build Requirements
  - CMake 3.5 or later (download from [https://cmake.org/](https://cmake.org/))
  - gcc-aarch64-linux-gnu 
  - binutils-aarch64-linux-gnu

- Build Instructions
  ```
  $mkdir build-arm
  $cd build-arm
  $cmake .. -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_SYSTEM_PROCESSOR=aarch64 -DARM=TRUE
  $make
  ```
  - Output Location
    - Executable application (xevd_app) can be found under build-arm/bin/.
    - Library files (libxevd.so and libxevd.a) can be found under build-arm/lib/.

## How to generate installer

### Linux (64-bit)
- Generation of **DEB packages** instructions
  - Follow build instruction and build the project
  - Generate **DEB packages**
    ```
    $make package
    ```
    or
    ```
    cpack -G "DEB"
    ```
  - Output
    - Base DEB package for **Baseline Profile**:
      - package: xevd-base-dev_1.0.0_amd64.deb
      - checksum file: xevd-base-dev_1.0.0_amd64.deb.md5
    - Developer DEB package for **Baseline Profile**::
      - package: xevd-base_1.0.0_amd64.deb
      - checksum file: xevd-base_1.0.0_amd64.deb.md5 generated.
    - Base DEB package for **Main Profile**:
      - package: xevd-main-dev_1.0.0_amd64.deb
      - checksum file: xevd-main-dev_1.0.0_amd64.deb.md5
    - Developer DEB package for **Main Profile**:
      - package: xevd-main_1.0.0_amd64.deb
      - checksum file: xevd-base_1.0.0_amd64.deb.md5 generated.

- Generation of **RPM packages**
  -  Follow build instruction and build the project
  -  Generate **RPM packages**
     ```
     cpack -G "RPM" ..
     ```

- Generation of **ZIP archives**
  -  Follow build instruction and build the project
  -  Generate **ZIP archive**
     ```
     cpack -G "ZIP" ..
     ```

### Windows (64-bit)
- Requirements
  - NSIS 3.08 or later (download from [https://nsis.sourceforge.io/Download](https://nsis.sourceforge.io/Download))

- Generation of **NSIS windows installer** instructions
  - Follow build instruction and build the project
  - Generate **NSIS Windows installer**
    - Command Prompt for Visual Studio
      - Go to the build directory and issue the following command
      ```
      msbuild /P:Configuration=Release PACKAGE.vcxproj
      ```

    - Visual Studio IDE
      - Open up the generated solution (XEVD.sln)
      - Change build type from Debug to Release
      - Go to the Solution Explorer, then select and mouse right click on the PACKAGE project located in CMakePredefinedTargets folder
      - Choose Build item, when a pop down menu appears

      > As a result CPack processing message should appear and NSIS installer as well as as checksum file are generated into build directory.

    - MinGW-64
      - Go to the build directory and issue the following command
      ```
      $make package
      ```
  - Output:
    - Baseline Profile:
      - xevd-base-1.0.0-win64.exe
      - xevd-base-1.0.0-win64.exe.md5

    - Main Profile:
        - xevd-main-1.0.0-win64.exe
        - xevd-main-1.0.0-win64.exe.md5
## How to use
XEVD supports main and baseline profiles of EVC.

| OPTION                | DEFAULT   | DESCRIPTION                                    |
|-----------------------|-----------|------------------------------------------------|
| -i, --input           | -         | file name of input bitstream                   |
| -o, --output          | -         | file name of output video                      |
| -m, --threads         | 1         | mumber of threads to be created                |


>More options can be found when type **xevd_app** only.

### Example
	$xevd_app -i input_bitstream.evc -o output_video.yuv

## License
See [COPYING](COPYING) file for details.
