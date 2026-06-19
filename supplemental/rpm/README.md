# Fedora RPM packaging

This directory contains Fedora-oriented RPM packaging for the Beszel agent.

Build locally from the repository root:

```bash
make rpm
```

The build creates source and binary RPMs under `build/rpm/`.

Install the agent package:

```bash
sudo dnf install ./build/rpm/RPMS/*/beszel-agent-*.rpm
```

The package installs the systemd unit but does not enable or start it
automatically (per Fedora preset policy).

Agent configuration lives in `/etc/beszel-agent/beszel-agent.conf`. The file is
owned by `root:beszel` with mode `0640` because it may contain secrets. At
minimum, set `KEY` before starting `beszel-agent.service`. For websocket mode,
also configure `HUB_URL` and `TOKEN`.

The `beszel` service user is created from `/usr/lib/sysusers.d/beszel-agent.conf`
with its home/state directory at `/var/lib/beszel-agent`.

SELinux notes:

- The binary is installed to `/usr/bin`, so Fedora labels it as a normal executable.
- State is kept under `/var/lib/beszel-agent`.
- Agent secrets can be stored directly in `/etc/beszel-agent/beszel-agent.conf`,
  which is installed with restricted permissions.

Example agent setup:

```bash
sudoedit /etc/beszel-agent/beszel-agent.conf
sudo systemctl enable --now beszel-agent.service
```
