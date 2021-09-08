# Installation guide

Installation of the *Gateway Process* consists of 2 steps:
1. Install dependencies
2. Building the *Gateway Process*

## Step 1: Install dependencies
````
sudo apt update
sudo apt install ssh g++ git libboost-atomic-dev libboost-thread-dev libboost-system-dev libboost-date-time-dev libboost-regex-dev libboost-filesystem-dev libboost-random-dev libboost-chrono-dev libboost-serialization-dev libwebsocketpp-dev openssl libssl-dev ninja-build libspdlog-dev libmbedtls-dev libboost-all-dev libconfig++-dev libsctp-dev libfftw3-dev vim libcpprest-dev libusb-1.0-0-dev net-tools smcroute python-psutil python3-pip clang-tidy gpsd gpsd-clients libgps-dev libgmime-3.0-dev libtinyxml2-dev libtinyxml2-6a
sudo snap install cmake --classic
sudo pip3 install cpplint
````

## Step 2: Building the Gateway Process

### 2.1 Getting the source code

````
cd ~
git clone --recurse-submodules https://github.com/5G-MAG/obeca-gateway-process.git

cd obeca-gateway-process

git submodule update

mkdir build && cd build
````

### 2.2 Build setup
`` cmake -DCMAKE_INSTALL_PREFIX=/usr -GNinja .. ``

Alternatively, to configure a debug build:
`` cmake -DCMAKE_INSTALL_PREFIX=/usr -GNinja -DCMAKE_BUILD_TYPE=Debug .. ``

### 2.3 Building
`` ninja ``

### 2.4 Installing
`` sudo ninja install `` 

The application, like the Receive Process, also installs a systemd unit.
