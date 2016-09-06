Name:       nemo-qml-plugin-devicelock
Summary:    Device lock plugin for Nemo Mobile
Version:    0.0.6
Release:    1
Group:      System/Libraries
License:    LGPLv2.1
URL:        https://git.merproject.org/mer-core/nemo-qml-plugin-dbus
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(keepalive)
BuildRequires:  pkgconfig(mce)
BuildRequires:  pkgconfig(mlite5)
BuildRequires:  pkgconfig(libsystemd-daemon)

%description
%{summary}.

%package default
Summary:    The default lock code based device lock plugin for Nemo Mobile
Group:      System/GUI/Other
Provides:   nemo-qml-plugin-devicelock = %{version}-%{release}

%description default
%{summary}.

%package devel
Summary:    Development libraries for device lock QML plugins
Group:      Development/Libraries

%description devel
%{summary}.

%prep
%setup -q -n %{name}-%{version}

%build
rm .qmake.cache || true
%qmake5
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%qmake5_install

%files default
%defattr(-,root,root,-)
%dir %{_libdir}/qt5/qml/org/nemomobile/devicelock
%{_libdir}/qt5/qml/org/nemomobile/devicelock/libnemodevicelockplugin.so
%{_libdir}/qt5/qml/org/nemomobile/devicelock/qmldir

%files devel
%dir %{_includedir}/nemo-devicelock
%{_includedir}/nemo-devicelock/*.h
%{_libdir}/libnemodevicelock.a
%{_libdir}/pkgconfig/nemodevicelock.pc
