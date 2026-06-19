# Fedora RPM packaging

This directory contains Fedora-oriented RPM packaging for Beszel.

Build locally from the repository root:

```bash
make rpm
```

The build creates source and binary RPMs under `build/rpm/`.

Install one or both subpackages:

```bash
sudo dnf install ./build/rpm/RPMS/*/beszel-hub-*.rpm
sudo dnf install ./build/rpm/RPMS/*/beszel-agent-*.rpm
```

The packages install systemd units but do not enable or start them automatically.

Hub configuration lives in `/etc/sysconfig/beszel-hub`. The default hub service
listens on `0.0.0.0:8090`.

Agent configuration lives in `/etc/beszel-agent/beszel-agent.conf`. The file is
owned by `root:beszel` with mode `0640` because it may contain secrets. At
minimum, set `KEY` before starting `beszel-agent.service`. For websocket mode,
also configure `HUB_URL` and `TOKEN`.

SELinux notes:

- Binaries are installed to `/usr/bin`, so Fedora labels them as normal executables.
- State is kept under `/var/lib/beszel-hub` and `/var/lib/beszel-agent`.
- Agent secrets can be stored directly in `/etc/beszel-agent/beszel-agent.conf`,
  which is installed with restricted permissions.

Example agent setup:

```bash
sudoedit /etc/beszel-agent/beszel-agent.conf
```
