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
BuildRequires:	openssl
BuildRequires:	openssl-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description 
myMPD is a standalone webclient for mpd.

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

%files 
%defattr(-,root,root,-)
%doc README.md LICENSE
/usr/bin/mympd
/usr/share/mympd
%config /etc/mympd
/usr/share/man/man1/mympd.1.gz

%changelog
* Tue Aug 28 2018 Juergen Mang <mail@jcgames.de> - master
- Version from master
