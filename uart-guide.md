# Zen Cape UART Guide – Kernel 4.9+
by Jeffrey Leung, Rulai Hu, Bryce Haley, Paymon Jalali

___

_Last update:_ November 5, 2019

_Version note:_ This guide targets boards which have the cape management built into UBoot, not the Linux Cape Manager (which is now removed).

This document guides the user through:
1. Enabling the UART serial port on the BeagleBone Green.
2. Connecting two BeagleBone greens through their serial ports.
3. Communication between the two BeagleBones through serial.

_Formatting:_
1. Host (desktop) commands starting with `$` are Linux console commands:
```shell
$ echo "Hello world"
```
2. Target (board) commands start with `#`:
```shell
# echo "On embedded board"
```
3. Almost all commands are case sensitive.

___

## 1. Installing Virtual Audio Cape

For Linux to use the UART hardware, we must activate a device tree file to tell the kernel how to access the hardware.

If a guide uses the $SLOTS file or the cape manager then that guide is out of date and does not apply to kernel 4.9+.

Linux must be told what hardware is connected to the CPU. It learns this at boot up using a Device Tree (file is a .DTB for the device tree binary). The boot loader (UBoot) detects what capes are installed (or configured) and sets up the device tree for the kernel to use.

Do the following just once (per board).

0. First, check to make sure the UART port is not yet enabled:
```
# ls -l /dev/ttyO*
```
Only one result should appear (`/dev/ttyO0`) which is the default serial console that we usually access. We will need to enable another serial port.
If you see the other serial ports, then you can skip section 1.

1. Update device tree overlays:
The BeagleBone ships with some device tree overlay ( .dtbo ) files; however, they are out of date. Update them:
```shell
# sudo apt update
# sudo apt install bb-cape-overlays
```

2. Backup uEnv.txt:
The uEnv.txt file is critical to controlling how UBoot starts the system. We will change it to load the UART component; however, we must first take a backup copy.

Make a backup copy of the uEnv file like so
```
# sudo cp /boot/uEnv.txt /boot/uEnv-Before-Uart.txt
```

WARNING: FAILING TO TAKE A BACKUP COPY COULD MAKE YOUR BOARD UN-BOOTABLE AND REQUIRE REFLASHING IF ANYTHING GOES WRONG.

3. Edit uEnv.txt to load the overlays

The Device Tree Overlay for the UART port is `/lib/firmware/BB-UART2-00A0.dtbo`. Check to make sure it exists:
```shell
# ls -l /lib/firmware/*UART*
```

You should see output like the following:
```shell
-rw-r--r-- 1 root root  867 Jan 28  2018 /lib/firmware/ADAFRUIT-UART1-00A0.dtbo
-rw-r--r-- 1 root root  867 Jan 28  2018 /lib/firmware/ADAFRUIT-UART2-00A0.dtbo
-rw-r--r-- 1 root root  867 Jan 28  2018 /lib/firmware/ADAFRUIT-UART4-00A0.dtbo
-rw-r--r-- 1 root root  867 Jan 28  2018 /lib/firmware/ADAFRUIT-UART5-00A0.dtbo
-rw-r--r-- 1 root root 1233 Oct 17 21:20 /lib/firmware/BB-UART1-00A0.dtbo
-rw-r--r-- 1 root root 1283 Oct 17 21:20 /lib/firmware/BB-UART1-RTSCTS-00A0.dtbo
-rw-r--r-- 1 root root 1233 Oct 17 21:20 /lib/firmware/BB-UART2-00A0.dtbo
-rw-r--r-- 1 root root 1368 Oct 17 21:20 /lib/firmware/BB-UART2-RTSCTS-00A0.dtbo
-rw-r--r-- 1 root root 1169 Oct 17 21:20 /lib/firmware/BB-UART3-00A0.dtbo
-rw-r--r-- 1 root root 1233 Oct 17 21:20 /lib/firmware/BB-UART4-00A0.dtbo
-rw-r--r-- 1 root root 1531 Oct 17 21:20 /lib/firmware/BB-UART4-RS485-00A0.dtbo
-rw-r--r-- 1 root root 1283 Oct 17 21:20 /lib/firmware/BB-UART4-RTSCTS-00A0.dtbo
-rw-r--r-- 1 root root 1233 Oct 17 21:20 /lib/firmware/BB-UART5-00A0.dtbo
-rw-r--r-- 1 root root 1283 Oct 17 21:20 /lib/firmware/BB-UART5-RTSCTS-00A0.dtbo
-rw-r--r-- 1 root root 1688 Oct 17 21:20 /lib/firmware/BB-mBC2-UART1-TESEO-LIV3F.dtbo
-rw-r--r-- 1 root root 1632 Oct 17 21:20 /lib/firmware/BB-mBC3-UART1-TESEO-LIV3F.dtbo
-rw-r--r-- 1 root root 1632 Oct 17 21:20 /lib/firmware/BB-mBC4-UART4-TESEO-LIV3F.dtbo
-rw-r--r-- 1 root root  820 Oct 17 21:20 /lib/firmware/PB-UART4-GNSS-4-CLICK.dtbo
-rw-r--r-- 1 root root 1226 Oct 17 21:20 /lib/firmware/PB-UART4-GNSS-5-CLICK.dtbo
-rw-r--r-- 1 root root 1627 Oct 17 21:20 /lib/firmware/PB-UART4-TESEO-LIV3F.dtbo
```


Edit the uEnv.txt file:
```shell
# sudo nano /boot/uEnv.txt
```

Change the line which reads:
```shell
#uboot_overlay_addr6=/lib/firmware/<file6>.dtbo
```
to use the appropriate UART port:
```shell
uboot_overlay_addr6=/lib/firmware/BB-UART2-00A0.dtbo
```

Ensure you removed the # on the first two lines to un-comment them!

4. Reboot the target.

5. After rebooting, the new serial port should be accessible:

```shell
# ls -ltr /dev/ttyO2
```
