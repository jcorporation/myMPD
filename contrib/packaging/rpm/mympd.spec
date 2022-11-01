#
# spec file for package myMPD
#
# (c) 2018-2022 Juergen Mang <mail@jcgames.de>

Name:           mympd
Version:        10.1.0
Release:        0
License:        GPL-3.0-or-later
Group:          Productivity/Multimedia/Sound/Players
Summary:        A standalone and mobile friendly web-based MPD client
Url:            https://jcorporation.github.io/myMPD/
Packager:       Juergen Mang <mail@jcgames.de>
Source:         mympd-%{version}.tar.gz
BuildRequires:  cmake
BuildRequires:	flac-devel
BuildRequires:  gcc
BuildRequires:  libid3tag-devel
BuildRequires:  lua-devel
BuildRequires:  openssl-devel
BuildRequires:  pcre2-devel
BuildRequires:  perl
BuildRequires:  pkgconfig
BuildRequires:  unzip
BuildRequires:  gzip
BuildRequires:  jq
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
myMPD is a standalone and lightweight web-based MPD client.
It's tuned for minimal resource usage and requires only very few dependencies.
Therefore myMPD is ideal for raspberry pis and similar devices.

%debug_package

%prep
%setup -q -n %{name}-%{version}

%build
install -d release
cd release || exit 1
cmake -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=Release -DMYMPD_STRIP_BINARY=OFF ..
make

%install
cd release || exit 1
make install DESTDIR=%{buildroot}

%post
echo "Checking status of mympd system user and group"
getent group mympd > /dev/null || groupadd -r mympd
getent passwd mympd > /dev/null || useradd -r -g mympd -s /bin/false -d /var/lib/mympd mympd
echo "myMPD installed"
true

%postun
if [ "$1" = "0" ]
then
  echo "Please purge /var/lib/mympd manually"
fi

%files
%defattr(-,root,root,-)
%doc README.md
/usr/bin/mympd
/usr/bin/mympd-script
/usr/lib/systemd/system/mympd.service
%{_mandir}/man1/mympd.1.gz
%{_mandir}/man1/mympd-script.1.gz
%license LICENSE.md

%changelog
* Thu Oct 20 2022 Juergen Mang <mail@jcgames.de> 10.1.0-0
- Version from master
