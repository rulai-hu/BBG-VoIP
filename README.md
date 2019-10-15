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