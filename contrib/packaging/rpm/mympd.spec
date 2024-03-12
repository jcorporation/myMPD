#
# spec file for package myMPD
#
# (c) 2018-2024 Juergen Mang <mail@jcgames.de>

Name:           mympd
<<<<<<< HEAD
Version:        15.0.0
=======
Version:        14.1.0
>>>>>>> devel
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
Requires: whiptail
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
myMPD is a standalone and lightweight web-based MPD client.
It's tuned for minimal resource usage and requires only very few dependencies.
Therefore myMPD is ideal for raspberry pis and similar devices.

%if 0%{?disturl:1}
  # build debug package in obs
  %debug_package
%endif

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
%{_mandir}/man1/mympd.1.gz
%{_mandir}/man1/mympd-config.1.gz
%{_mandir}/man1/mympd-script.1.gz
%{_defaultdocdir}/mympd/CHANGELOG.md
%{_defaultdocdir}/mympd/LICENSE.md
%{_defaultdocdir}/mympd/README.md
%{_defaultdocdir}/mympd/SECURITY.md
%license LICENSE.md

%changelog
<<<<<<< HEAD
* Sun Mar 03 2024 Juergen Mang <mail@jcgames.de> 15.0.0-0
=======
* Tue Mar 12 2024 Juergen Mang <mail@jcgames.de> 14.1.0-0
>>>>>>> devel
- Version from master
