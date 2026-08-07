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
  mkdir build
  cd build
  cmake ..
  make
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
    mkdir build
    cd build
    cmake .. -G "MinGW Makefiles"
    make
    ```
  - Microsoft Visual Studio
    ```
    mkdir build
    cd build
    cmake .. -G "Visual Studio 15 2017 Win64"
    make
    ```
    You can change '-G' option with proper version of Visual Studio.

### ARM (64-bit)
On an aarch64 host no special option is needed: the architecture is detected
automatically, so the Linux instructions above apply as-is. Architectures
without SIMD support build automatically with a plain C fallback.
The instructions below are for cross-compiling on an x86 host.

- Build Requirements
  - CMake 3.5 or later (download from [https://cmake.org/](https://cmake.org/))
  - gcc-aarch64-linux-gnu 
  - binutils-aarch64-linux-gnu

- Build Instructions
  ```
  mkdir build-arm
  cd build-arm
  cmake .. -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc -DCMAKE_SYSTEM_PROCESSOR=aarch64 -DARM=TRUE
  make
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
    make package
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
      make package
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
| -m, --threads         | 1         | number of threads to be created                |


>More options can be found when type **xevd_app** only.

### Example
	xevd_app -i input_bitstream.evc -o output_video.yuv

## Programming Guide
The following code is a pseudo code for understanding how to use the library
```c
#include <xevd.h>

XEVD_CDSC cdsc;
memset(&cdsc, 0, sizeof(XEVD_CDSC));
cdsc.threads = 1;

XEVD id = xevd_create(&cdsc, NULL);

XEVD_BITB bitb; /* one nal unit per call */
XEVD_STAT stat;
XEVD_IMGB *imgb;

while (read_nal_unit(&bitb))
{
    xevd_decode(id, &bitb, &stat);
    if (stat.fnum >= 0 && xevd_pull(id, &imgb) == XEVD_OK)
    {
        write_image(imgb);
        imgb->release(imgb);
    }
}
/* flush remaining (reordered) pictures with xevd_pull(), then clean up */
xevd_delete(id);
```

### Reading SEI payloads
SEI payloads found in the access unit of a picture (e.g. HDR metadata) are
exposed on the picture returned by `xevd_pull()`. The memory belongs to the
library and is valid until the picture is released, so copy the payloads
before calling `imgb->release()`.
```c
if (imgb->ndata[XEVD_IMGB_SEI_SLOT] == XEVD_SEI_MAGIC)
{
    XEVD_SEI *sei = (XEVD_SEI *)imgb->pdata[XEVD_IMGB_SEI_SLOT];
    for (int i = 0; i < sei->num_payloads; i++)
    {
        XEVD_SEI_PAYLOAD *p = &sei->payloads[i];
        /* p->payload_type, p->payload_size, p->payload */
    }
}
```

## How to contribute
Contributions are welcome through GitHub pull requests.

* Fork the repository and create a topic branch from `master`.
* Keep each PR focused; put unrelated fixes into separate PRs.
* Make sure both profiles still build before submitting (`-DSET_PROF=MAIN` and `-DSET_PROF=BASE`).
* Sign off your commits (`git commit -s`).
* **Please enable "Allow edits from maintainers" when opening a pull request.** This lets maintainers push small fixes (build tweaks, rebases, style cleanups) directly to your branch instead of going through another round of review comments, which can significantly shorten the review cycle.

## License
See [COPYING](COPYING) file for details.
