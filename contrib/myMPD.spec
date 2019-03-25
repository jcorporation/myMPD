#
# spec file for package myMPD
#
# (c) 2018-2019 Juergen Mang <mail@jcgames.de>

Name:           myMPD
Version:        5.2.0
Release:        0 
License:        GPL-2.0 
Group:          Productivity/Multimedia/Sound/Players
Summary:        Standalone webclient for mpd
Url:            https://github.com/jcorporation/myMPD
Source:         https://github.com/jcorporation/myMPD/archive/v%{version}.zip
BuildRequires:  gcc
BuildRequires:	cpp
BuildRequires:  cmake
BuildRequires:  unzip
BuildRequires:	libmpdclient-devel
BuildRequires:	pkgconfig
BuildRequires:	openssl-devel
BuildRequires:  libmediainfo-devel
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
echo "Checking status of mympd system user and group"
getent group mympd > /dev/null
[ "$?" = "2" ] && groupadd -r mympd
getent passwd mympd > /dev/null
[ "$?" = "2" ] && useradd -r mympd -g mympd -d /var/lib/mympd -s /usr/sbin/nologin

if [ -d /etc/systemd ]
then
  [ -d /usr/lib/systemd/system ] || mkdir -p /usr/lib/systemd/system 
  cp /usr/share/mympd/mympd.service /usr/lib/systemd/system/
  systemctl daemon-reload
  systemctl enable mympd
fi

# move smartpls into place unless already existing
for PLDIST in /var/lib/mympd/smartpls/*.dist
do
  if [ -f "$PLDIST" ]
  then
    PLS=$(basename $PLDIST .dist)
    if [ -f /var/lib/mympd/smartpls/$PLS ]
    then
      rm $PLDIST
    else
      mv -v $PLDIST /var/lib/mympd/smartpls/$PLS
    fi
  fi
done

echo "Fixing ownership of /var/lib/mympd"
chown -R mympd.mympd /var/lib/mympd

# move config into place unless already existing
if [ ! -f /etc/mympd/mympd.conf ]
then 
  mv /etc/mympd/mympd.conf.dist /etc/mympd/mympd.conf
else
  echo "mympd.conf installed as mympd.conf.dist"
fi
  
/usr/share/mympd/crcert.sh

%postun
if [ "$1" = "0" ]
then
  if [ -f /usr/lib/systemd/system/mympd.service ]
  then
    if `systemctl is-active --quiet mympd`
    then
      echo "stopping mympd.service" && systemctl stop mympd 
    fi
    echo "disabling mympd.service" && systemctl disable mympd
    rm -v -f /usr/lib/systemd/system/mympd.service
    systemctl daemon-reload
  fi
fi

%files 
%defattr(-,root,root,-)
%doc README.md LICENSE
/usr/bin/mympd
/usr/share/mympd
%config /etc/mympd
/usr/share/man/man1/mympd.1.gz
/var/lib/mympd

%changelog
* Mon Mar 25 2019 Juergen Mang <mail@jcgames.de> - master
- Version from master
