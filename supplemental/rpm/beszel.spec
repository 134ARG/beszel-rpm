Name:           beszel-agent
Version:        0.18.7
Release:        1%{?dist}
Summary:        Lightweight server monitoring agent

License:        MIT
URL:            https://github.com/henrygd/beszel
Source0:        beszel-%{version}.tar.gz

%global debug_package %{nil}

BuildRequires:  golang
BuildRequires:  systemd-rpm-macros
Requires(pre):  shadow-utils
%{?systemd_requires}

%description
Beszel is a lightweight server monitoring platform. This package provides the
Beszel agent service and binary.

%prep
%autosetup -n beszel-%{version}

%build
# Match the upstream goreleaser agent build: CGO disabled, glibc tag enabled
# for NVML GPU support on glibc-based distributions.
export CGO_ENABLED=0
# Build from the release tarball, which excludes VCS metadata; disable VCS
# stamping so the build does not fail trying to read git state.
go build -buildmode=pie -buildvcs=false -tags glibc -ldflags "-w -s" -o build/beszel-agent ./internal/cmd/agent

%install
install -Dpm 0755 build/beszel-agent %{buildroot}%{_bindir}/beszel-agent
install -Dpm 0644 supplemental/rpm/beszel.sysusers %{buildroot}%{_sysusersdir}/beszel-agent.conf
install -Dpm 0644 supplemental/rpm/beszel-agent.service %{buildroot}%{_unitdir}/beszel-agent.service
install -Dpm 0640 supplemental/rpm/beszel-agent.conf %{buildroot}%{_sysconfdir}/beszel-agent/beszel-agent.conf

%post
%systemd_post beszel-agent.service

%preun
%systemd_preun beszel-agent.service

%postun
%systemd_postun_with_restart beszel-agent.service

%files
%license LICENSE
%doc readme.md supplemental/rpm/README.md
%{_bindir}/beszel-agent
%{_sysusersdir}/beszel-agent.conf
%{_unitdir}/beszel-agent.service
%dir %attr(0750,root,beszel) %{_sysconfdir}/beszel-agent
%config(noreplace) %attr(0640,root,beszel) %{_sysconfdir}/beszel-agent/beszel-agent.conf

%changelog
* Mon May 04 2026 Beszel RPM Maintainers <noreply@example.com> - 0.18.7-1
- Add Fedora RPM packaging with a systemd unit for the agent.
