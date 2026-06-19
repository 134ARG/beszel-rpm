//go:build linux

package agent

import (
	"context"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/henrygd/beszel/agent/utils"
	"github.com/henrygd/beszel/internal/entities/system"

	"github.com/shirou/gopsutil/v4/common"
)

// updateHumidities reads relative humidity sensors from hwmon (e.g. SHT4x) and
// stores them on systemStats. gopsutil's sensors package only exposes
// temp*_input, so humidity*_input is collected directly from sysfs here.
func (a *Agent) updateHumidities(systemStats *system.Stats) {
	if a.sensorConfig.skipCollection {
		return
	}

	sysPath := hostSysPath(a.sensorConfig.context)
	files, err := filepath.Glob(filepath.Join(sysPath, "/class/hwmon/hwmon*/humidity*_input"))
	if err != nil {
		return
	}
	if len(files) == 0 {
		files, _ = filepath.Glob(filepath.Join(sysPath, "/class/hwmon/hwmon*/device/humidity*_input"))
	}
	if len(files) == 0 {
		return
	}

	humidities := make(map[string]float64, len(files))
	for i, file := range files {
		directory := filepath.Dir(file)
		basename := strings.Split(filepath.Base(file), "_")[0] // e.g. humidity1
		basepath := filepath.Join(directory, basename)

		// chip name (e.g. sht4x)
		raw, err := os.ReadFile(filepath.Join(directory, "name"))
		if err != nil {
			continue
		}
		name := strings.TrimSpace(string(raw))

		// optional label, formatted like gopsutil ("Inlet" -> "inlet")
		if raw, _ = os.ReadFile(basepath + "_label"); len(raw) != 0 {
			label := strings.Join(strings.Fields(strings.ToLower(string(raw))), "_")
			if label != "" {
				name = name + "_" + label
			}
		}

		// humidity reading is in milli-percent RH
		if raw, err = os.ReadFile(file); err != nil {
			continue
		}
		value, err := strconv.ParseFloat(strings.TrimSpace(string(raw)), 64)
		if err != nil {
			continue
		}
		value = value / 1000.0

		// skip implausible values
		if value <= 0 || value > 100 {
			continue
		}

		sensorName := name
		if _, ok := humidities[sensorName]; ok {
			sensorName = sensorName + "_" + strconv.Itoa(i)
		}
		if !isValidSensor(sensorName, a.sensorConfig) {
			continue
		}
		humidities[sensorName] = utils.TwoDecimals(value)
	}

	if len(humidities) > 0 {
		systemStats.Humidities = humidities
	}
}

// hostSysPath returns the sysfs root, honoring the SYS_SENSORS override stored
// in the sensor context (gopsutil common.EnvMap), then the HOST_SYS env var,
// defaulting to "/sys".
func hostSysPath(ctx context.Context) string {
	if v := ctx.Value(common.EnvKey); v != nil {
		if env, ok := v.(common.EnvMap); ok {
			if p, ok := env[common.HostSysEnvKey]; ok && p != "" {
				return p
			}
		}
	}
	if p := os.Getenv(string(common.HostSysEnvKey)); p != "" {
		return p
	}
	return "/sys"
}
