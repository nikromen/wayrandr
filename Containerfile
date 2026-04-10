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
    kanshi \
    dnf-plugins-core \
    python3 \
    python3-pip \
    clang \
    clang-tools-extra \
    compiler-rt \
    glibc-langpack-cs \
    gcovr \
    grim \
    && dnf clean all

RUN pip3 install pre-commit

RUN dnf copr enable -y nikromen/auto-wlr-randr \
    && dnf install -y auto-wlr-randr \
    && dnf clean all

WORKDIR /workspace

CMD ["/bin/bash"]
