#
# spec file for package myMPD
#
# (c) 2018-2025 Juergen Mang <mail@jcgames.de>

Name:           mympd
Version:        22.1.2
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
BuildRequires:  gzip
BuildRequires:  jq
BuildRequires:  libid3tag-devel
BuildRequires:  lua-devel
BuildRequires:  openssl-devel
BuildRequires:  pcre2-devel
BuildRequires:  perl
BuildRequires:  pkgconfig
BuildRequires:  unzip
Requires: newt
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
myMPD is a standalone and lightweight web-based MPD client.
It's tuned for minimal resource usage and requires only very few dependencies.
Therefore myMPD is ideal for raspberry pis and similar devices.

%prep
%setup -q -n %{name}-%{version}

%build
cmake -B release -DCMAKE_INSTALL_PREFIX:PATH=/usr -DCMAKE_BUILD_TYPE=RelWithDebInfo .
make -C release

%install
make -C release install DESTDIR=%{buildroot}
if [ "%{_defaultdocdir}" == "/usr/share/doc/packages" ]
then
  install -d "%{buildroot}%{_defaultdocdir}"
  mv -v "%{buildroot}/usr/share/doc/mympd" "%{buildroot}%{_defaultdocdir}/mympd"
fi

%files
%defattr(-,root,root,-)
%doc README.md
/usr/bin/mympd
/usr/bin/mympd-config
/usr/bin/mympd-script
/usr/lib/systemd/system/mympd.service
/usr/lib/systemd/user/mympd.service
%{_mandir}/man1/mympd.1.gz
%{_mandir}/man1/mympd-config.1.gz
%{_mandir}/man1/mympd-script.1.gz
%{_defaultdocdir}/mympd/CHANGELOG.md
%{_defaultdocdir}/mympd/LICENSE.md
%{_defaultdocdir}/mympd/README.md
%{_defaultdocdir}/mympd/SECURITY.md
%license LICENSE.md

%changelog
* Sat Oct 11 2025 Juergen Mang <mail@jcgames.de> 22.1.2-0
- Version from master
