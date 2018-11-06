# OdysseyWebBrowser

Notes about branches:

master  - always going to be empty
webkit  - branch that contains original webkit codes at certain revisions
odyssey - branch that contains Odyssey Web Browser rebased onto certain version of webkit branch

In order to build/develop the Odyssey Web Browser, checkout branch odyssey.

#How to compile
The compilaton was successfully tested using Ubuntu 14.04.5 LTS 32 bit. Below the list of packages installed. In order to compile you need to apply the provided patch. Here the steps I followed.

```
svn co --username guest --password guest https://svn.aros.org/svn/aros/branches/ABI_V0-20161228
git clone https://github.com/deadwood-pl/OdysseyWebBrowser.git
cd OdysseyWebBrowser.git
git checkout odyssey-r187682
cd ..

cd ABI_V0-20161228/AROS
mkdir local
cd local
ln -s ../../OdysseyWebBrowser.git odyssey
cd ..
ln -s ../contrib
patch -p0 -i ../OdysseyWebBrowser/ABI_V0-20161228-patch/ABI_V0-20161228.diff

../ABI_V0-20161228/AROS/configure --target=linux-i386 --with-portssources=/home/nicola/PortSources --with-paranoia=no --with-gcc-version=4.8.3 --with-binutils-version=2.25
make

make local-odyssey
```

##Setup of VM
Starting from a base installation of Ubuntu 14.04.5 LTS 32 bit Server execute following commands:
```
sudo apt update
sudo apt install -y unzip

#Packages metioned in gimmearos.sh
sudo apt install -y subversion
sudo apt install -y git-core
sudo apt install -y gcc
sudo apt install -y g++
sudo apt install -y make
sudo apt install -y gawk
sudo apt install -y bison
sudo apt install -y flex
sudo apt install -y bzip2
sudo apt install -y netpbm
sudo apt install -y autoconf
sudo apt install -y automake
sudo apt install -y libx11-dev
sudo apt install -y libxext-dev
sudo apt install -y libc6-dev
sudo apt install -y liblzo2-dev
sudo apt install -y libxxf86vm-dev
sudo apt install -y libpng12-dev
sudo apt install -y gcc-multilib
sudo apt install -y libsdl1.2-dev
sudo apt install -y byacc
sudo apt install -y cmake

#Additional requirement to compile (not mentioned in gimmearos)
apt -y install libgl1-mesa-dev xorg-dev
apt -y install cmake
apt -y install gperf
apt -y install libswitch-perl

apt -y install texinfo
apt -y install uml-utilities

apt -y install vnc4server
apt -y install ruby
apt -y install ruby-dev

#Packages required to run AROS
dpkg --add-architecture i386
apt update
apt -y install libxcursor1:i386
apt -y install libxxf86vm1:i386

#Optional: install xorg and openbox
apt -y xorg openbox

# Change X11 resolution 
# xrandr --output VGA-0 --mode 1280x960
```
