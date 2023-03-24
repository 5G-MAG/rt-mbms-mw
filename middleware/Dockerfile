FROM ubuntu:20.04 as BUILDER
RUN apt-get update && \ 
    apt-get install sudo && DEBIAN_FRONTEND=noninteractive \
    apt-get install -y \
    tzdata \
    ssh g++ git libboost-atomic-dev \
    libboost-thread-dev libboost-system-dev \
    libboost-date-time-dev libboost-regex-dev \
    libboost-filesystem-dev libboost-random-dev \
    libboost-chrono-dev libboost-serialization-dev \
    libwebsocketpp-dev openssl libssl-dev \
    ninja-build libspdlog-dev libmbedtls-dev \
    libboost-all-dev libconfig++-dev libsctp-dev \
    libfftw3-dev vim libcpprest-dev libusb-1.0-0-dev \
    net-tools smcroute python-psutil python3-pip \
    clang-tidy gpsd gpsd-clients libgps-dev \
    libgmime-3.0-dev libtinyxml2-dev \
    libtinyxml2-6a 
RUN sudo apt-get -y install cmake
RUN sudo pip3 install cpplint

RUN mkdir -p /opt/install

#Building middleware------
RUN mkdir -p /opt/build/rt-mbms-mw 
WORKDIR /opt/build/rt-mbms-mw
RUN git clone --recurse-submodules https://github.com/5G-MAG/rt-mbms-mw . && \
    git submodule update 
RUN mkdir build && cd build && \
    cmake -DCMAKE_INSTALL_PREFIX=/usr -GNinja .. && \
    ninja && sudo ninja install && \
    sudo ldconfig
RUN rm /etc/5gmag-rt.conf
COPY 5gmag-rt.conf /etc/
WORKDIR /
EXPOSE 3020/udp 3020 80
ENTRYPOINT [ "/usr/bin/mw"]

