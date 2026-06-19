# Optional Fedora SMART hardening

This directory contains an opt-in way to collect S.M.A.R.T. data without giving
`CAP_SYS_ADMIN` or `CAP_SYS_RAWIO` to the long-running Beszel agent process.

The practical design is:

- `beszel-agent` runs without ambient SMART capabilities.
- Beszel finds `/usr/libexec/beszel/smartctl` before `/usr/sbin/smartctl`.
- `/usr/libexec/beszel/smartctl` is a small argument-filtering wrapper.
- Only the wrapper has file capabilities.
- The wrapper allows Beszel's read-style `smartctl` calls, raises the required
  capabilities for the child process, then executes `/usr/sbin/smartctl`.

This is safer than granting the capabilities directly to `beszel-agent`, but it
is still privileged and should remain opt-in.

## Files

- `wrappers/beszel-smartctl-wrapper.c` - validating `smartctl` launcher.

## Deploy on Fedora

Install dependencies:

```bash
sudo dnf install smartmontools gcc libcap
```

Build and install the wrapper:

```bash
gcc -O2 -Wall -Wextra -o beszel-smartctl-wrapper supplemental/security/wrappers/beszel-smartctl-wrapper.c
sudo install -D -o root -g beszel -m 0750 beszel-smartctl-wrapper /usr/libexec/beszel/smartctl
```

Start with the smaller capability set:

```bash
sudo setcap cap_dac_read_search,cap_sys_rawio+eip /usr/libexec/beszel/smartctl
```

If NVMe SMART collection fails with permission or ioctl errors, add
`CAP_SYS_ADMIN`:

```bash
sudo setcap cap_dac_read_search,cap_sys_rawio,cap_sys_admin+eip /usr/libexec/beszel/smartctl
```

Create a systemd override:

```bash
sudo systemctl edit beszel-agent.service
```

Use:

```ini
[Service]
Environment=PATH=/usr/libexec/beszel:/usr/sbin:/usr/bin
AmbientCapabilities=
CapabilityBoundingSet=CAP_DAC_READ_SEARCH CAP_SYS_RAWIO CAP_SYS_ADMIN
NoNewPrivileges=no
```

`CapabilityBoundingSet` intentionally keeps these capabilities available to the
service tree, but `AmbientCapabilities` is empty so `beszel-agent` does not
receive them. The file-capability wrapper acquires them only when it is
executed.

Reload and restart:

```bash
sudo systemctl daemon-reload
sudo systemctl restart beszel-agent.service
```

## Verify

Check wrapper ownership and capabilities:

```bash
ls -lZ /usr/libexec/beszel/smartctl
getcap /usr/libexec/beszel/smartctl
```

Check the service does not have ambient capabilities:

```bash
systemctl show beszel-agent.service -p AmbientCapabilities -p CapabilityBoundingSet -p NoNewPrivileges
```

Check the wrapper accepts expected read-style commands:

```bash
sudo -u beszel /usr/libexec/beszel/smartctl --scan -j
sudo -u beszel /usr/libexec/beszel/smartctl -a --json=c /dev/sda
sudo -u beszel /usr/libexec/beszel/smartctl -d nvme -a --json=c /dev/nvme0
```

Check the wrapper rejects unsafe or unexpected commands:

```bash
sudo -u beszel /usr/libexec/beszel/smartctl --smart=off /dev/sda
sudo -u beszel /usr/libexec/beszel/smartctl -t short /dev/sda
```

The rejected commands should exit with status `126`.

## Agent configuration

To avoid automatic scans, set the devices explicitly in
`/etc/beszel-agent/beszel-agent.conf`:

```ini
SMART_DEVICES=/dev/sda:sat,/dev/nvme0:nvme
```

## Notes

The wrapper still permits `ioctl`, because SMART/NVMe health queries require it.
It also uses `CAP_DAC_READ_SEARCH` so `/usr/sbin/smartctl` can open root-owned
disk device nodes for reading without adding `beszel` to the broad `disk` group.
This reduces risk by keeping broad capabilities away from the long-running
network-facing agent and limiting the accepted `smartctl` argument shapes.

If `systemctl show beszel-agent.service -p NoNewPrivileges` reports `yes`, the
wrapper will not be able to acquire file capabilities. Confirm the override was
applied.
