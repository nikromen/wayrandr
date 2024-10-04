%global desc %{expand:
Wayrandr is a simple GUI for kanshi and wlr-randr. It allows you to easily
manage your display configuration.
}

Name:           wayrandr
Version:        0.1.0
Release:        %autorelease
Summary:        Simple QT kanshi and wlr-randr GUI

License:        GPL-3.0-or-later
URL:            https://github.com/nikromen/%{name}
Source0:        %{url}/archive/refs/tags/%{name}-%{version}.tar.gz


BuildArch:      noarch

BuildRequires:  python3-devel
BuildRequires:  pyproject-rpm-macros

Requires:       python3-pyside6


%description
%{desc}


%prep
%autosetup


%generate_buildrequires
%pyproject_buildrequires -r


%build
%pyproject_wheel


%install
%pyproject_install
%pyproject_save_files %{name}


%files -f %{pyproject_files}
%license LICENSE
%doc README.md
%{_bindir}/%{name}


%changelog
%autochangelog
