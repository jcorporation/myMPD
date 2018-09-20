#
# spec file for package myMPD
#
# (c) 2018 Juergen Mang <mail@jcgames.de

Name:           myMPD
Version:        4.2.1
Release:        0 
License:        GPL-2.0 
Group:          Productivity/Multimedia/Sound/Players
Summary:        Standalone webclient for mpd
Url:            https://github.com/jcorporation/myMPD
Source:         https://github.com/jcorporation/myMPD/archive/v4.2.1.zip
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
echo "Checking status of mympd system user and group"
getent group mympd > /dev/null
[ "$?" = "2" ] && groupadd -r mympd
getent passwd mympd > /dev/null
[ "$?" = "2" ] && useradd -r mympd -g mympd -d /var/lib/mympd -s /usr/sbin/nologin

if ! [ $(stat -c '%U:%G' /var/lib/mympd/) == 'mympd:mympd' ]
then
  echo "Fixing ownership of /var/lib/mympd"
  chown -R mympd.mympd /var/lib/mympd
fi

SYSTEMCTL=$(which systemctl > /dev/null 2>&1)
if [ "$?" == "0" ]
then
  [ -d /usr/lib/systemd/system ] || sudo mkdir -p /usr/lib/systemd/system 
  cp /usr/share/mympd/mympd.service /usr/lib/systemd/system/
  systemctl daemon-reload
  systemctl enable mympd
fi

if [ -f /etc/mpd.conf ]
then
  LIBRARY=$(grep ^music_directory /etc/mpd.conf | awk {'print $2'} | sed -e 's/"//g')
  [ "$LIBRARY" != "" ] && [ ! -e /usr/share/mympd/htdocs/library ] && ln -s "$LIBRARY" /usr/share/mympd/htdocs/library
fi

if [ ! -f /etc/mympd/mympd.conf ]
then 
  mv /etc/mympd/mympd.conf.dist /etc/mympd/mympd.conf
else
  echo "mympd.conf installed as mympd.conf.dist"
fi
  
/usr/share/mympd/crcert.sh

%postun
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

rm -v -fr /var/lib/mympd
[ -e /usr/share/mympd/htdocs/library ] && rm -v /usr/share/mympd/htdocs/library
rmdir -v /usr/share/{mympd/htdocs/,mympd/}

getent passwd mympd > /dev/null
echo "Removing mympd user and group"
[ "$?" != "2" ] && userdel -r mympd
getent group mympd > /dev/null
[ "$?" != "2" ] && groupdel mympd


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
