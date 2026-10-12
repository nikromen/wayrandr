%global srcname wayrandr

%if 0%{?git_build}
%global pkg_name %{srcname}-git
%else
%global pkg_name %{srcname}
%endif

Name:           %{pkg_name}
Version:        1.0
Release:        %autorelease
Summary:        Graphical display configuration for Wayland compositors

License:        GPL-3.0-or-later
URL:            https://github.com/nikromen/wayrandr
Source0:        %{url}/archive/refs/tags/%{srcname}-%{version}.tar.gz

BuildRequires:  gcc-c++
BuildRequires:  cmake
BuildRequires:  cmake(Qt6Core)
BuildRequires:  cmake(Qt6Gui)
BuildRequires:  cmake(Qt6Quick)
BuildRequires:  cmake(Qt6QuickControls2)
BuildRequires:  cmake(Qt6Test)
BuildRequires:  cmake(Qt6QuickTest)
BuildRequires:  cmake(nlohmann_json)
BuildRequires:  cmake(spdlog)
BuildRequires:  cmake(tomlplusplus)
BuildRequires:  pkgconfig(scfg) >= 0.2.0
BuildRequires:  python3
BuildRequires:  glibc-langpack-cs
BuildRequires:  desktop-file-utils

Requires:       qt6-qtdeclarative
Requires:       qt6-qtquickcontrols2
Requires:       qt6-qtwayland
Requires:       wlr-randr
Requires:       grim
Suggests:       kanshi
Suggests:       auto-wlr-randr

%if 0%{?git_build}
Conflicts:      %{srcname}
%else
Conflicts:      %{srcname}-git
%endif

%description
Wayrandr provides a graphical interface for configuring displays on Wayland
compositors supporting the wlr-output-management protocol. It supports direct
display configuration through wlr-randr and profile editing for the optional
kanshi and auto-wlr-randr daemons.
%if 0%{?git_build}

This is a development build from the main branch.
%endif

%prep
%autosetup -n %{srcname}-%{srcname}-%{version}

%build
%cmake \
    -DWAYRANDR_USE_SYSTEM_DEPENDENCIES=ON \
    -DBUILD_TESTING=ON \
    -DENABLE_DEBUG_LOGS=OFF \
    -DWAYRANDR_FLATPAK_PROBES=OFF
%cmake_build

%install
%cmake_install

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/%{srcname}.desktop
%ctest --output-on-failure --no-tests=error

%files
%license LICENSE
%{_bindir}/%{srcname}
%{_datadir}/applications/%{srcname}.desktop

%changelog
%autochangelog
