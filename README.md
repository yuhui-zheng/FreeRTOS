## Note on the branch qemu_posix_LM3S6965
This branch is a show case for FreeRTOS+POSIX running on top of QEMU LM3S6965 port. This is not an official release.

Project is located at ```/FreeRTOS/FreeRTOS/Demo/CORTEX_LM3Sxxxx_IAR_Keil/RTOSDemo.uvproj```.

**Keil IDE**: Keil MDK 5.29

**ARM compiler**: ARMCC/ARMASM/.. V5.06 update 6 (build 750)

**C dialect**: C99

**QEMU version**: 4.02, running on Windows 64-bit

### To reproduce:
- compile project. 
- open a terminal window, cd into 

```
<path to repository>/FreeRTOS/FreeRTOS/Demo/CORTEX_LM3Sxxxx_IAR_Keil
```

- in the terminal window, start QEMU with 

```
qemu-system-arm -machine lm3s6965evb -s -kernel rvmdk/RTOSDemo.axf
```

You shall see QEMU window pops up. This demo uses both emulated OLED and UART. OLED view could be accessed via "View -> ssd0302"; while UART view could be accessed via "View -> serial0". Demo periodically writes to OLED, and only POSIX tasks write to UART (UART log will stop once POSIX demo finishes). 

### To debug your application, similar as to above procedure:
- open two terminal windows, cd into 

```
<path to repository>/FreeRTOS/FreeRTOS/Demo/CORTEX_LM3Sxxxx_IAR_Keil
```

- in a terminal window, start QEMU with 

```
qemu-system-arm -machine lm3s6965evb -s -S -kernel rvmdk/RTOSDemo.axf
```

- assume you have [ARM toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm) installed. in a second terminal window, start gdb with

```
 arm-none-eabi-gdb -q ./rvmdk/RTOSDemo.axf
```
Step as you would normally do with GDB.



## Cloning this repository
This repo uses [Git Submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules) to bring in dependent components.

Note: If you download the ZIP file provided by GitHub UI, you will not get the contents of the submodules. (The ZIP file is also not a valid git repository)

To clone using HTTPS:
```
git clone https://github.com/FreeRTOS/FreeRTOS.git --recurse-submodules
```
Using SSH:
```
git clone git@github.com:FreeRTOS/FreeRTOS.git --recurse-submodules
```

If you have downloaded the repo without using the `--recurse-submodules` argument, you need to run:
```
git submodule update --init --recursive
```
