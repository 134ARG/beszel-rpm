// Minimal smartctl argument filter for Beszel.
//
// Install as /usr/libexec/beszel/smartctl, grant file capabilities to this
// wrapper, and put /usr/libexec/beszel first in beszel-agent.service's PATH.
// This intentionally allows only the read-style smartctl invocations currently
// used by the Beszel agent:
//
//   smartctl --scan -j
//   smartctl [-d TYPE] -a --json=c [-l devstat] [-n standby] /dev/DEVICE

#include <ctype.h>
#include <errno.h>
#include <linux/capability.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define REAL_SMARTCTL "/usr/sbin/smartctl"

static void deny(const char *reason)
{
	fprintf(stderr, "beszel-smartctl-wrapper: denied: %s\n", reason);
	exit(126);
}

static int streq(const char *a, const char *b)
{
	return strcmp(a, b) == 0;
}

static int valid_device_type(const char *value)
{
	if (value == NULL || value[0] == '\0') {
		return 0;
	}
	for (const unsigned char *p = (const unsigned char *) value; *p; p++) {
		if (!(isalnum(*p) || *p == '_' || *p == '-' || *p == ',')) {
			return 0;
		}
	}
	return 1;
}

static int valid_device_path(const char *value)
{
	if (value == NULL || strncmp(value, "/dev/", 5) != 0) {
		return 0;
	}
	if (strstr(value, "..") != NULL) {
		return 0;
	}
	for (const unsigned char *p = (const unsigned char *) value; *p; p++) {
		if (!(isalnum(*p) || *p == '/' || *p == '_' || *p == '-' || *p == '.')) {
			return 0;
		}
	}
	return 1;
}

static void validate_scan(int argc, char **argv)
{
	if (argc == 3 && streq(argv[1], "--scan") && streq(argv[2], "-j")) {
		return;
	}
	deny("unsupported scan arguments");
}

static void validate_collect(int argc, char **argv)
{
	int saw_all = 0;
	int saw_json = 0;
	int saw_device = 0;

	for (int i = 1; i < argc; i++) {
		if (streq(argv[i], "-d")) {
			if (++i >= argc || !valid_device_type(argv[i])) {
				deny("invalid -d value");
			}
			continue;
		}
		if (streq(argv[i], "-a")) {
			saw_all = 1;
			continue;
		}
		if (streq(argv[i], "--json=c")) {
			saw_json = 1;
			continue;
		}
		if (streq(argv[i], "-l")) {
			if (++i >= argc || !streq(argv[i], "devstat")) {
				deny("only -l devstat is allowed");
			}
			continue;
		}
		if (streq(argv[i], "-n")) {
			if (++i >= argc || !streq(argv[i], "standby")) {
				deny("only -n standby is allowed");
			}
			continue;
		}
		if (valid_device_path(argv[i])) {
			if (saw_device) {
				deny("multiple device paths");
			}
			saw_device = 1;
			continue;
		}
		deny("unsupported argument");
	}

	if (!saw_all || !saw_json || !saw_device) {
		deny("collect requires -a --json=c and one /dev path");
	}
}

static void set_cap_bit(struct __user_cap_data_struct data[2], int cap, __u32 flag)
{
	int index = cap / 32;
	int bit = cap % 32;
	data[index].effective |= flag << bit;
	data[index].permitted |= flag << bit;
	data[index].inheritable |= flag << bit;
}

static int cap_is_permitted(const struct __user_cap_data_struct data[2], int cap)
{
	int index = cap / 32;
	int bit = cap % 32;
	return (data[index].permitted & (1U << bit)) != 0;
}

static void raise_ambient_cap(int cap, const char *name)
{
	if (prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, cap, 0, 0) != 0) {
		fprintf(stderr, "beszel-smartctl-wrapper: raise %s failed: %s\n", name, strerror(errno));
		exit(127);
	}
}

static void prepare_ambient_caps(void)
{
	struct __user_cap_header_struct header = {
		.version = _LINUX_CAPABILITY_VERSION_3,
		.pid = 0,
	};
	struct __user_cap_data_struct data[2] = { 0 };
	int has_dac_read_search;
	int has_sys_rawio;
	int has_sys_admin;

	if (syscall(SYS_capget, &header, data) != 0) {
		fprintf(stderr, "beszel-smartctl-wrapper: capget failed: %s\n", strerror(errno));
		exit(127);
	}

	has_dac_read_search = cap_is_permitted(data, CAP_DAC_READ_SEARCH);
	has_sys_rawio = cap_is_permitted(data, CAP_SYS_RAWIO);
	has_sys_admin = cap_is_permitted(data, CAP_SYS_ADMIN);

	if (has_dac_read_search) {
		set_cap_bit(data, CAP_DAC_READ_SEARCH, 1U);
	}
	if (has_sys_rawio) {
		set_cap_bit(data, CAP_SYS_RAWIO, 1U);
	}
	if (has_sys_admin) {
		set_cap_bit(data, CAP_SYS_ADMIN, 1U);
	}

	if (syscall(SYS_capset, &header, data) != 0) {
		fprintf(stderr, "beszel-smartctl-wrapper: capset failed: %s\n", strerror(errno));
		exit(127);
	}

	if (has_dac_read_search) {
		raise_ambient_cap(CAP_DAC_READ_SEARCH, "CAP_DAC_READ_SEARCH");
	}
	if (has_sys_rawio) {
		raise_ambient_cap(CAP_SYS_RAWIO, "CAP_SYS_RAWIO");
	}
	if (has_sys_admin) {
		raise_ambient_cap(CAP_SYS_ADMIN, "CAP_SYS_ADMIN");
	}
}

int main(int argc, char **argv)
{
	if (argc < 2) {
		deny("missing arguments");
	}

	if (streq(argv[1], "--scan")) {
		validate_scan(argc, argv);
	} else {
		validate_collect(argc, argv);
	}

	prepare_ambient_caps();
	execv(REAL_SMARTCTL, argv);
	fprintf(stderr, "beszel-smartctl-wrapper: exec %s failed: %s\n", REAL_SMARTCTL, strerror(errno));
	return 127;
}
