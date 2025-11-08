FROM fedora:latest

RUN dnf install -y \
    cmake \
    make \
    gcc \
    gcc-c++ \
    qt6-qtbase-devel \
    qt6-qtdeclarative-devel \
    qt6-qtquickcontrols2-devel \
    git \
    wayland-devel \
    wlr-randr \
    python3 \
    python3-pip \
    clang \
    clang-tools-extra \
    grim \
    && dnf clean all

RUN pip3 install pre-commit

WORKDIR /workspace

CMD ["/bin/bash"]
