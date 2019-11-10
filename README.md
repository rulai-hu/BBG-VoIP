# CMPT 433 Project: Tin Can Telephones

BeagleBone program which connects two devices over remote communication through VoIP (Voice over Internet Protocol).

## Setup

### Beaglebone

Connect your BeagleBone to the host computer.

Create the directory `~/cmpt433/public/myApps/` on the host computer:
```shell
mkdir -p ~/cmpt433/public/myApps/
```

Mount a remote Network File Server (NFS) from the BeagleBone to the directory above.

### The Program

Clone the GitHub repository:
```shell
git clone csil-git1.cs.surrey.sfu.ca/hurulaih/cmpt433-project.git
```

Enter the repository directory:
```shell
cd cmpt433-project/
```

Build the executable and send it to the BeagleBone through the NFS:
```shell
make
```

SSH into the BeagleBone and run the executable:
```shell
./tincanphone
```

## Schematic

A Fritzing schematic is provided in the directory `schematic/`.

![Schematic](readme-img/schematic.jpg)

To edit the Fritzing file, download [Fritzing](https://fritzing.org/download/).

Open Fritzing, select **File**, and choose **Open**. Select the library `schematic/AdaFruit.fzbz` to load the BeagleBone part.

Select **File** and choose **Open** to load the schematic.

## Installing PortAudio on BBG

This section is still a work in progress.

### Prerequisites
* ALSA must be installed and configured on the target.
  * Then overwrite the ALSA config: `cp alsa.conf /usr/share/alsa/alsa.conf`
* The target must have internet access for the rest to work.

If you haven't installed libasound2-dev on the target:
* `apt-get install libasound2-dev`

Download, make and configure:
* `cd /mnt/remote/`
* `wget pa_stable_v190600_20161030.tgz`
* `tar xvzf pa_stable_v190600_20161030.tgz`
* `./configure && make`

**Important**. Ensure that PortAudio is correctly configured by checking the configuration summary. You should see something like this (the important line has been bolded):

```
Configuration summary:

  Target ...................... armv7l-unknown-linux-gnueabihf
  C++ bindings ................ no
  Debug output ................ no

  **ALSA ........................ yes**
  ASIHPI ...................... no

  OSS ......................... yes
  JACK ........................ no

```
