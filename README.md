# CMPT 433 Project: Tin Can Telephones

BeagleBone and ZenCape program which connects two devices over remote communication through VoIP (Voice over Internet Protocol). This is a project for _CMPT 433: Embedded Systems_ at Simon Fraser University in Fall 2019.

![Project image](readme-img/board.jpg)

## Detailed Overview

This project uses [PortAudio](http://www.portaudio.com/) for recording sound and [Advanced Linux Sound Architecture (ALSA)](https://alsa-project.org/) for playing and managing sound through a wired USB headset.

The volume is controlled by turning the potentiometer on the ZenCape.

**Hardware components:**
  * Beaglebone Green
  * ZenCape
  * Wired USB headset
  * [12-digit keypad](https://www.rpelectronics.com/12key-12-key-keypad-common-ground.html)
  * Potentiometer

**Creators:** Rulai Hu, Bryce Haley, Paymon Jalali, Jeffrey Leung

## Setup

### Beaglebone

Connect your BeagleBone to the host computer.

Create the directory `~/cmpt433/public/myApps/` on the host computer:
```shell
$ mkdir -p ~/cmpt433/public/myApps/
```

Mount a remote Network File Server (NFS) from the BeagleBone to the directory above.

### The Program

Clone the GitHub repository:
```shell
$ git clone csil-git1.cs.surrey.sfu.ca/hurulaih/cmpt433-project.git
```

Enter the repository directory:
```shell
$ cd cmpt433-project/
```

Build the executable and send it to the BeagleBone through the NFS:
```shell
$ make
```

SSH into the BeagleBone and run the executable:
```shell
./tincanphone
```

## Schematic

A Fritzing schematic is provided in the directory `schematic/`.

![Schematic](schematic/schematic.png)

To edit the Fritzing file, download [Fritzing](https://fritzing.org/download/).

Open Fritzing, select **File**, and choose **Open**. Select the library `schematic/AdaFruit.fzbz` to load the BeagleBone part.

Select **File** and choose **Open** to load the schematic.

## Installing PortAudio on the BBG

### Prerequisites
* ALSA must be installed and configured on the target.
* The target must have internet access enabled

SSH or screen into the target machine.

Install library `libasound2-dev`:
```shell
apt-get install libasound2-dev
```

Overwrite the ALSA config with the one provided in our repository (but not before making a copy first):
```shell
cp /usr/share/alsa/alsa.conf /usr/share/alsa/alsa.conf.before-tincantelephones
cp alsa.conf /usr/share/alsa/alsa.conf
```

Download, make and configure PortAudio **on the target**:
```shell
cd /mnt/remote/
wget http://portaudio.com/archives/pa_stable_v190600_20161030.tgz
tar xvzf pa_stable_v190600_20161030.tgz
./configure && make
```

**Important**: Ensure that PortAudio is correctly configured by checking the configuration summary. You should see something like this:

```
Configuration summary:

  Target ...................... armv7l-unknown-linux-gnueabihf
  C++ bindings ................ no
  Debug output ................ no

  ALSA ........................ yes
  ASIHPI ...................... no

  OSS ......................... yes
  JACK ........................ no
```

It is vital that ALSA is **yes**. Otherwise, `libasound2-dev` has not been correctly installed on the target.

### Troubleshooting

Are you experiencing:

* Segfaults, or "unable to open stream" errors?
  * Make sure the correct device index is being targeted. For the audio jacks, it's **0**. For USB headphones, it's **1**.
  * `make device_info` and then run the `device_info` program on the target. If you don't see a list of devices (more than zero) then ALSA isn't set up correctly.

* A whole bunch of runtime errors such as `ALSA lib pcm.c:2495:(snd_pcm_open_noupdate) Unknown PCM surround71`?
  * Make sure the default `alsa.conf` is overwritten with the one provided and the errors will go away.
