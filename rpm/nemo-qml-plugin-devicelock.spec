Name:       nemo-qml-plugin-devicelock
Summary:    Device lock plugin for Nemo Mobile
Version:    0.3.8
Release:    1
License:    BSD and LGPLv2
URL:        https://git.sailfishos.org/mer-core/nemo-qml-plugin-devicelock
Source0:    %{name}-%{version}.tar.bz2
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Network)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(keepalive)
BuildRequires:  pkgconfig(libsystemd)
BuildRequires:  pkgconfig(mce)
BuildRequires:  pkgconfig(nemodbus)
BuildRequires:  sailfish-minui-devel >= 0.0.25
Obsoletes:      nemo-qml-plugin-devicelock-default < 0.2.0
Requires:       nemo-devicelock-daemon

%description
%{summary}.

%package -n nemo-devicelock-daemon-cli
Summary:    The default command line security code device lock daemon for Nemo Mobile
Requires:   %{name} = %{version}-%{release}
Provides:   nemo-devicelock-daemon = %{version}-%{release}

%description -n nemo-devicelock-daemon-cli
%{summary}.

%package devel
Summary:    Development libraries for device lock
Requires:   %{name} = %{version}-%{release}
Requires:   pkgconfig(nemodbus)

%description devel
%{summary}.

%package host-devel
Summary:    Development libraries for device lock daemons
Requires:   %{name}-devel = %{version}-%{release}
Requires:   pkgconfig(keepalive)
Requires:   pkgconfig(libsystemd)
Requires:   pkgconfig(mce)
Requires:   pkgconfig(nemodbus)

%description host-devel
%{summary}.

%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 "VERSION=%{version}"
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
%qmake5_install

mkdir -p %{buildroot}%{_unitdir}/multi-user.target.wants
ln -sf ../nemo-devicelock.socket %{buildroot}%{_unitdir}/multi-user.target.wants/

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libnemodevicelock.so.*
%dir %{_libdir}/qt5/qml/org/nemomobile/devicelock
%{_libdir}/qt5/qml/org/nemomobile/devicelock/libnemodevicelockplugin.so
%{_libdir}/qt5/qml/org/nemomobile/devicelock/plugins.qmltypes
%{_libdir}/qt5/qml/org/nemomobile/devicelock/qmldir
%{_unitdir}/nemo-devicelock.socket
%{_unitdir}/multi-user.target.wants/nemo-devicelock.socket
%config %{_sysconfdir}/dbus-1/system.d/org.nemomobile.devicelock.conf
%license LICENSE.LGPL

%files -n nemo-devicelock-daemon-cli
%defattr(-,root,root,-)
%{_libexecdir}/nemo-devicelock
%{_unitdir}/nemo-devicelock.service

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
