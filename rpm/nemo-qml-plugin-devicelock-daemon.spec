Name:       nemo-qml-plugin-devicelock-daemon
Summary:    QML plugin for Nemo Mobile device lock daemon
Version:    0.0.0
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
Provides:   nemo-qml-plugin-devicelock
Conflicts:  nemo-qml-plugin-devicelock-default
Requires:   nemo-devicelock-daemon

%description
%{summary}.

%package -n nemo-devicelock-daemon-cli
Summary:    The default command line lock code device lock daemon for Nemo Mobile
Group:      System/GUI/Other
Provides:   nemo-devicelock-daemon

%description -n nemo-devicelock-daemon-cli
%{summary}.

%prep
%setup -q -n %{name}-%{version}

%build
echo DEVICELOCK_DAEMON=yes > .qmake.cache
%qmake5
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%qmake5_install

%files
%defattr(-,root,root,-)
%dir %{_libdir}/qt5/qml/org/nemomobile/devicelock
%{_libdir}/qt5/qml/org/nemomobile/devicelock/libnemodevicelockplugin.so
%{_libdir}/qt5/qml/org/nemomobile/devicelock/qmldir
%exclude %{_includedir}/nemo-devicelock/*.h
%exclude %{_libdir}/libnemodevicelock.a
%exclude %{_libdir}/pkgconfig/nemodevicelock.pc

%files -n nemo-devicelock-daemon-cli
%defattr(-,root,root,-)
%{_bindir}/nemodevicelockd
/lib/systemd/system/nemodevicelock.service
/lib/systemd/system/nemodevicelock.socket
%{_sysconfdir}/dbus-1/system.d/org.nemomobile.devicelock.conf
