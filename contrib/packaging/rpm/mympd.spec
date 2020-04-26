#
# spec file for package myMPD
#
# (c) 2018-2020 Juergen Mang <mail@jcgames.de>

Name:           mympd
Version:        6.3.1
Release:        0 
License:        GPL-2.0-or-later
Group:          Productivity/Multimedia/Sound/Players
Summary:        Standalone web mpd client
Url:            https://github.com/jcorporation/myMPD
Source:         mympd-%{version}.tar.gz
BuildRequires:  gcc
BuildRequires:  cmake
BuildRequires:  unzip
BuildRequires:  pkgconfig
BuildRequires:  openssl-devel
BuildRequires:  libid3tag-devel
BuildRequires:	flac-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%global debug_package %{nil}

%description 
myMPD is a standalone and mobile friendly web mpdclient.

%prep 
%setup -q -n %{name}-%{version}

%build
mkdir release
cd release || exit 1
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=RELEASE ..
make

%install
cd release || exit 1
make install DESTDIR=%{buildroot}

%post
echo "Checking status of mympd system user and group"
getent group mympd > /dev/null || groupadd -r mympd
getent passwd mympd > /dev/null || useradd -r -g mympd -s /bin/false -d /var/lib/mympd mympd
true

%postun
if [ "$1" = "0" ]
then
  echo "Please purge /var/lib/mympd manually"
fi

%files 
%defattr(-,root,root,-)
%doc README.md LICENSE
/usr/bin/mympd
/usr/bin/mympd-config
/usr/lib/systemd/system/mympd.service
%config(noreplace) /etc/mympd.conf

%changelog
* Sun Apr 26 2020 Juergen Mang <mail@jcgames.de> 6.3.1-0
- Version from master
