
FROM ubuntu:18.04


RUN apt update && \
    apt -y install git && \
    apt -y install cmake && \
    apt -y install wget && \
    apt -y install gpg && \
    apt -y install xz-utils


RUN wget https://github.com/devkitPro/pacman/releases/download/devkitpro-pacman-1.0.1/devkitpro-pacman.deb && \
    dpkg -i devkitpro-pacman.deb && \
    dkp-pacman -S --noconfirm gba-dev


ENV DEVKITPRO=/opt/devkitpro
ENV DEVKITARM=${DEVKITPRO}/devkitARM
ENV DEVKITPPC=${DEVKITPRO}/devkitPPC


WORKDIR /root
RUN git clone https://github.com/evanbowman/blind-jump-portable.git


WORKDIR /root/blind-jump-portable/build
RUN cmake -DCMAKE_TOOLCHAIN_FILE=$(pwd)/devkitarm.cmake .


ENTRYPOINT ["/bin/bash"]
