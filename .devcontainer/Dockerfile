FROM almalinux:9

RUN dnf install -y epel-release && \
    dnf install -y gcc make flex bison git clang-tools-extra cppcheck && \
    dnf clean all
