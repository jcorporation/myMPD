#
# spec file for package myMPD
#
# (c) 2018 Juergen Mang <mail@jcgames.de

Name:           myMPD
Version:        4.2.0
Release:        0 
License:        GPL-2.0 
Group:          Productivity/Multimedia/Sound/Players
Summary:        Standalone webclient for mpd
Url:            https://github.com/jcorporation/myMPD
Source:         https://github.com/jcorporation/myMPD/archive/v4.2.0.zip
BuildRequires:  gcc
BuildRequires:  cmake
BuildRequires:  unzip
BuildRequires:	libmpdclient-devel
BuildRequires:	pkgconfig
BuildRequires:	openssl-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%global debug_package %{nil}

%description 
myMPD is a standalone and mobile friendly web mpdclient.

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

%post
getent group mympd > /dev/null
[ "$?" = "2" ] && groupadd -r mympd
getent passwd mympd > /dev/null
[ "$?" = "2" ] && useradd -r mympd -g mympd -d /var/lib/mympd -s /usr/sbin/nologin
if [ -d /usr/lib/systemd/ ]
then
  [ -d /usr/lib/systemd/system ] || sudo mkdir /usr/lib/systemd/system 
  cp /usr/share/mympd/mympd.service /usr/lib/systemd/system/
fi
if [ -f /etc/mpd.conf ]
then
  LIBRARY=$(grep ^music_directory /etc/mpd.conf | awk {'print $2'} | sed -e 's/"//g')
  [ "$LIBRARY" != "" ] && [ ! -e /usr/share/mympd/htdocs/library ] && ln -s "$LIBRARY" /usr/share/mympd/htdocs/library
fi
chown -R mympd /var/lib/mympd
/usr/share/mympd/crcert.sh

%files 
%defattr(-,root,root,-)
%doc README.md LICENSE
/usr/bin/mympd
/usr/share/mympd
%config /etc/mympd
/usr/share/man/man1/mympd.1.gz
/var/lib/mympd

%changelog
* Wed Sep 17 2018 Juergen Mang <mail@jcgames.de> - master
- Version from master
