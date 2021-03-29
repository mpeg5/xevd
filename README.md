# eXtra-fast Essential Video Decoder (XEVD)
The **eXtra-fast Essential Video Decoder** (XEVD) is an opensource and fast MPEG-5 EVC decoder. 

**MPEG-5 Essential Video Coding** (EVC) is a future video compression standard of ISO/IEC Moving Picture Experts Grop (MPEG). The main goal of the EVC is to provide a significantly improved compression capability over existing video coding standards with timely publication of terms. 
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
    
## How to use
XEVD supports baseline profiles of EVC.

| OPTION                | DEFAULT   | DESCRIPTION                                    |
|-----------------------|-----------|------------------------------------------------|
| -i, --input           | -         | file name of input bitstream                   |
| -o, --output          | -         | file name of output video                      |
| -m, --parallel_task   | 1         | mumber of threads to be created                |  


>More optins can be found when type **xevdb_app** only.   
 
### Example
	$xevdb_app -i xevd.bin -o xevd.yuv


## License
See [COPYING](COPYING) file for details.
