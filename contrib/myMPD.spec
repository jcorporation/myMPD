#
# spec file for package myMPD
#
# (c) 2018 Juergen Mang <mail@jcgames.de

Name:           myMPD
Version:        master
Release:        0 
License:        GPL-2.0 
Group:          Productivity/Multimedia/Sound/Players
Summary:        Standalone webclient for mpd
Url:            https://github.com/jcorporation/myMPD
Source:         https://github.com/jcorporation/myMPD/archive/master.zip
BuildRequires:  gcc
BuildRequires:  cmake
BuildRequires:  unzip
BuildRequires:	libmpdclient-devel
BuildRequires:	libmpdclient2
BuildRequires:	pkgconfig
BuildRequires:	openssl-devel
BuildRequires:	systemd
%if 0%{?suse_version} >= 1210
BuildRequires: systemd-rpm-macros
%endif
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
%{?systemd_requires}

%description 
myMPD is a standalone and mobile friendly web mpdclient.

%pre
%if 0%{?suse_version}
%service_add_pre mympd.service
%endif

%prep 
%setup -q -n %{name}-%{version}

%build
mkdir release
cd release
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=RELEASE ..
make

%install
cd release
make install DESTDIR=%{buildroot}
chmod 755 %{buildroot}/usr/share/mympd/crcert.sh
install -D -m 644 %{buildroot}/debian/mympd.service %{buildroot}%{_unitdir}/mympd.service

%post
%if 0%{?rhel_version} || 0%{?centos_version}
%systemd_post mympd.service
%else
%service_add_post mympd.service
%endif

%preun
%if 0%{?rhel_version} || 0%{?centos_version}
%systemd_preun mympd.service
%else
%service_del_preun mympd.service
%endif

%postun
%if 0%{?rhel_version} || 0%{?centos_version}
%systemd_postun mympd.service
%else
%service_del_postun mympd.service
%endif

%files 
%defattr(-,root,root,-)
%doc README.md LICENSE
/usr/bin/mympd
/usr/share/mympd
%config /etc/mympd
/usr/share/man/man1/mympd.1.gz
/var/lib/mympd
%{_unitdir}/mympd.service

%changelog
* Tue Aug 28 2018 Juergen Mang <mail@jcgames.de> - master
- Version from master
