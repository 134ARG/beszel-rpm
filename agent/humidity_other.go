//go:build !linux

package agent

import "github.com/henrygd/beszel/internal/entities/system"

// updateHumidities is a no-op on non-linux platforms (hwmon humidity*_input
// is a Linux sysfs interface).
func (a *Agent) updateHumidities(systemStats *system.Stats) {}
