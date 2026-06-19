Name:           beszel
Version:        0.18.7
Release:        1%{?dist}
Summary:        Lightweight server monitoring hub and agent

License:        MIT
URL:            https://github.com/henrygd/beszel
Source0:        %{name}-%{version}.tar.gz

%global debug_package %{nil}

BuildRequires:  golang
BuildRequires:  npm
BuildRequires:  systemd-rpm-macros

%description
Beszel is a lightweight server monitoring platform. This package contains
common files shared by the Fedora systemd hub and agent packages.

%package hub
Summary:        Beszel monitoring hub
Requires(pre):  shadow-utils
%{?systemd_requires}

%description hub
Beszel hub service and binary.

%package agent
Summary:        Beszel monitoring agent
Requires(pre):  shadow-utils
%{?systemd_requires}

%description agent
Beszel agent service and binary.

%prep
%autosetup

%build
npm ci --prefix internal/site
npm run --prefix internal/site build

export CGO_ENABLED=1
go build -buildmode=pie -tags glibc -ldflags "-w -s" -o build/beszel ./internal/cmd/hub
go build -buildmode=pie -tags glibc -ldflags "-w -s" -o build/beszel-agent ./internal/cmd/agent

%install
install -Dpm 0755 build/beszel %{buildroot}%{_bindir}/beszel
install -Dpm 0755 build/beszel-agent %{buildroot}%{_bindir}/beszel-agent

install -Dpm 0644 supplemental/rpm/beszel.sysusers %{buildroot}%{_sysusersdir}/beszel.conf
install -Dpm 0644 supplemental/rpm/beszel-hub.service %{buildroot}%{_unitdir}/beszel-hub.service
install -Dpm 0644 supplemental/rpm/beszel-agent.service %{buildroot}%{_unitdir}/beszel-agent.service
install -Dpm 0644 supplemental/rpm/beszel-hub.sysconfig %{buildroot}%{_sysconfdir}/sysconfig/beszel-hub
install -Dpm 0640 supplemental/rpm/beszel-agent.conf %{buildroot}%{_sysconfdir}/beszel-agent/beszel-agent.conf

%pre hub
getent group beszel >/dev/null || groupadd -r beszel
getent passwd beszel >/dev/null || \
    useradd -r -g beszel -d /var/lib/beszel -s /usr/sbin/nologin \
    -c "Beszel service user" beszel
exit 0

%pre agent
getent group beszel >/dev/null || groupadd -r beszel
getent passwd beszel >/dev/null || \
    useradd -r -g beszel -d /var/lib/beszel -s /usr/sbin/nologin \
    -c "Beszel service user" beszel
exit 0

%post hub
%systemd_post beszel-hub.service

%preun hub
%systemd_preun beszel-hub.service

%postun hub
%systemd_postun_with_restart beszel-hub.service

%post agent
%systemd_post beszel-agent.service

%preun agent
%systemd_preun beszel-agent.service

%postun agent
%systemd_postun_with_restart beszel-agent.service

%files
%license LICENSE
%doc readme.md supplemental/rpm/README.md
%{_sysusersdir}/beszel.conf

%files hub
%license LICENSE
%{_sysusersdir}/beszel.conf
%{_bindir}/beszel
%{_unitdir}/beszel-hub.service
%config(noreplace) %{_sysconfdir}/sysconfig/beszel-hub

%files agent
%license LICENSE
%{_sysusersdir}/beszel.conf
%{_bindir}/beszel-agent
%{_unitdir}/beszel-agent.service
%dir %attr(0750,root,beszel) %{_sysconfdir}/beszel-agent
%config(noreplace) %attr(0640,root,beszel) %{_sysconfdir}/beszel-agent/beszel-agent.conf

%changelog
* Mon May 04 2026 Beszel RPM Maintainers <noreply@example.com> - 0.18.7-1
- Add Fedora RPM packaging with systemd units for hub and agent.
