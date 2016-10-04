Name:       nemo-qml-plugin-devicelock
Summary:    Device lock plugin for Nemo Mobile
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
BuildRequires:  pkgconfig(libsystemd-daemon))
BuildRequires:  pkgconfig(mce)
BuildRequires:  pkgconfig(nemodbus)
Obsoletes:      nemo-qml-plugin-devicelock-default < 0.2.0
Requires:       nemo-devicelock-daemon

%description
%{summary}.

%package -n nemo-devicelock-daemon-cli
Summary:    The default command line lock code device lock daemon for Nemo Mobile
Group:      System/GUI/Other
Requires:   %{name} = %{version}-%{release}
Provides:   nemo-devicelock-daemon = %{version}-%{release}

%description -n nemo-devicelock-daemon-cli
%{summary}.

%package devel
Summary:    Development libraries for device lock
Group:      Development/Libraries
Requires:   %{name} = %{version}-%{release}
Requires:   pkgconfig(nemodbus)

%description devel
%{summary}.

%package host-devel
Summary:    Development libraries for device lock daemons
Group:      Development/Libraries
Requires:   %{name}-devel = %{version}-%{release}
Requires:   pkgconfig(keepalive)
Requires:   pkgconfig(libsystemd-daemon)
Requires:   pkgconfig(mce)
Requires:   pkgconfig(nemodbus)

%description host-devel
%{summary}.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%qmake5_install

mkdir -p %{buildroot}/lib/systemd/system/multi-user.target.wants
ln -sf ../nemo-devicelock.socket %{buildroot}/lib/systemd/system/multi-user.target.wants/

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libnemodevicelock.so.*
%dir %{_libdir}/qt5/qml/org/nemomobile/devicelock
%{_libdir}/qt5/qml/org/nemomobile/devicelock/libnemodevicelockplugin.so
%{_libdir}/qt5/qml/org/nemomobile/devicelock/qmldir
/lib/systemd/system/nemo-devicelock.socket
/lib/systemd/system/multi-user.target.wants/nemo-devicelock.socket
%config %{_sysconfdir}/dbus-1/system.d/org.nemomobile.devicelock.conf

%files -n nemo-devicelock-daemon-cli
%defattr(-,root,root,-)
%{_libexecdir}/nemo-devicelock
/lib/systemd/system/nemo-devicelock.service

%files devel
%defattr(-,root,root,-)
%dir %{_includedir}/nemo-devicelock
%{_includedir}/nemo-devicelock/*.h
%{_includedir}/nemo-devicelock/private/*.h
%{_includedir}/nemo-devicelock/host/*.h
%{_libdir}/libnemodevicelock.so
%{_libdir}/pkgconfig/nemodevicelock.pc

%files host-devel
%defattr(-,root,root,-)
%dir %{_includedir}/nemo-devicelock/host
%{_includedir}/nemo-devicelock/host/*.h
%{_libdir}/libnemodevicelock-host.a
%{_datadir}/qt5/mkspecs/features/nemo-devicelock-host.prf
