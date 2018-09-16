Name: xtuple-openrpt
Version: 3.3.9
Release: 1%{?dist}
Summary: xTuple / PostBooks reporting utility and libraries
License: CPAL
Url: http://www.xtuple.com/openrpt/
Source: https://github.com/xtuple/openrpt/archive/v%version.tar.gz
BuildRequires: desktop-file-utils
BuildRequires: qt-devel
BuildRequires: libdmtx-devel
Requires: %{name}-libs%{?_isa} = %{version}-%{release}

%global _docdir_fmt %{name}

%description
Graphical SQL report writer, designer and rendering engine, optimized
for PostgreSQL. WYSIWYG display, GUI built with Qt. Reports can be saved
as XML, either as files or in a database.

%package images
BuildArch: noarch
Summary: Images for xTuple products

%description images
Graphical SQL report writer, designer and rendering engine, optimized
for PostgreSQL. WYSIWYG display, GUI built with Qt. Reports can be saved
as XML, either as files or in a database.
This package provides images used by OpenRPT and dependencies.

%package libs
Summary: Shared libraries for OpenRPT
Requires: %{name}-images = %{version}-%{release}

%description libs
Graphical SQL report writer, designer and rendering engine, optimized
for PostgreSQL. WYSIWYG display, GUI built with Qt. Reports can be saved
as XML, either as files or in a database.
This package provides the core libraries: libopenrpt

%package devel
Summary: OpenRPT development files
Requires: %{name}-libs%{?_isa} = %{version}-%{release}
Requires: qt-devel
Requires: libdmtx-devel

%description devel
Graphical SQL report writer, designer and rendering engine, optimized
for PostgreSQL. WYSIWYG display, GUI built with Qt. Reports can be saved
as XML, either as files or in a database.
This package provides the header files used by developers.

%prep
%setup -q -n openrpt-%{version}

%build
export USE_SYSTEM_DMTX=1
lrelease-qt4 */*/*.ts */*.ts
qmake-qt4 .
make %{?_smp_mflags}

%install
# make install doesn't do anything for this qmake project so we do
# the installs manually
#make INSTALL_ROOT=%{buildroot} install
#rm -f %{buildroot}%{_libdir}/lib*.a
mv bin/graph bin/openrpt-graph
mkdir -p %{buildroot}%{_bindir}
install bin/* %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_libdir}
cp -dp lib/lib*.so* %{buildroot}%{_libdir}
find %{buildroot}%{_libdir} -name 'lib*.so*' -exec chmod 0755 {} \;
mkdir -p %{buildroot}%{_includedir}/openrpt
find . -name '*.h' -exec install -m 0644 -D {} %{buildroot}%{_includedir}/openrpt/{} \;
mkdir -p %{buildroot}%{_datadir}/openrpt/OpenRPT/images
cp -r OpenRPT/images/* %{buildroot}%{_datadir}/openrpt/OpenRPT/images
rm %{buildroot}%{_datadir}/openrpt/OpenRPT/images/icons_24x24/Thumbs.db
rm %{buildroot}%{_datadir}/openrpt/OpenRPT/images/openrpt_qembed.h
mkdir -p %{buildroot}%{_datadir}/applications
desktop-file-install --dir=%{buildroot}%{_datadir}/applications *.desktop

%post libs -p /sbin/ldconfig

%postun libs -p /sbin/ldconfig

%files 
%{_bindir}/*
%{_datadir}/applications/*.desktop

%files libs
%{_libdir}/lib*.so.*

%files images
%license COPYING
%dir %{_datadir}/openrpt
%dir %{_datadir}/openrpt/OpenRPT
%dir %{_datadir}/openrpt/OpenRPT/images
%dir %{_datadir}/openrpt/OpenRPT/images/icons_24x24
%dir %{_datadir}/openrpt/OpenRPT/images/icons_48x48
%dir %{_datadir}/openrpt/OpenRPT/images/icons_32x32
%dir %{_datadir}/openrpt/OpenRPT/images/icons_64x64
%dir %{_datadir}/openrpt/OpenRPT/images/icons_16x16
%{_datadir}/openrpt/OpenRPT/images/*.*
%{_datadir}/openrpt/OpenRPT/images/icons_*/*

%files devel
%dir %{_includedir}/openrpt/
%{_includedir}/openrpt/*
%{_libdir}/libopenrptcommon.so
%{_libdir}/libMetaSQL.so
%{_libdir}/librenderer.so
%{_libdir}/libwrtembed.so

%changelog
* Wed Feb 25 2015 Daniel Pocock <daniel@pocock.pro> - 3.3.9-1
- Initial RPM packaging.

