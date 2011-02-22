/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2011 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#ifndef SDL_POWER_DISABLED
#ifdef SDL_POWER_LINUX

#include <stdio.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#include "SDL_power.h"

static const char *proc_apm_path = "/proc/apm";
static const char *proc_acpi_battery_path = "/proc/acpi/battery";
static const char *proc_acpi_ac_adapter_path = "/proc/acpi/ac_adapter";

static int open_acpi_file(const char *base, const char *node, const char *key)
{
    const size_t pathlen = strlen(base) + strlen(node) + strlen(key) + 3;
    char *path = (char *) alloca(pathlen);
    if (path == NULL) {
        return -1;  /* oh well. */
    }

    snprintf(path, pathlen, "%s/%s/%s", base, node, key);
    return open(path, O_RDONLY);
}


static SDL_bool
load_acpi_file(const char *base, const char *node, const char *key,
               char *buf, size_t buflen)
{
    ssize_t br = 0;
    const int fd = open_acpi_file(base, node, key);
    if (fd == -1) {
        return SDL_FALSE;
    }
    br = read(fd, buf, buflen-1);
    close(fd);
    if (br < 0) {
        return SDL_FALSE;
    }
    buf[br] = '\0';             // null-terminate the string.
    return SDL_TRUE;
}


static SDL_bool
make_proc_acpi_key_val(char **_ptr, char **_key, char **_val)
{
    char *ptr = *_ptr;

    while (*ptr == ' ') {
        ptr++;  /* skip whitespace. */
    }

    if (*ptr == '\0') {
        return SDL_FALSE;  /* EOF. */
    }

    *_key = ptr;

    while ((*ptr != ':') && (*ptr != '\0')) {
        ptr++;
    }

    if (*ptr == '\0') {
        return SDL_FALSE;  /* (unexpected) EOF. */
    }

    *(ptr++) = '\0';  /* terminate the key. */

    while ((*ptr == ' ') && (*ptr != '\0')) {
        ptr++;  /* skip whitespace. */
    }

    if (*ptr == '\0') {
        return SDL_FALSE;  /* (unexpected) EOF. */
    }

    *_val = ptr;

    while ((*ptr != '\n') && (*ptr != '\0')) {
        ptr++;
    }

    if (*ptr != '\0') {
        *(ptr++) = '\0';  /* terminate the value. */
    }

    *_ptr = ptr;  /* store for next time. */
    return SDL_TRUE;
}

static void
check_proc_acpi_battery(const char * node, SDL_bool * have_battery,
                        SDL_bool * charging, int *seconds, int *percent)
{
    const char *base = proc_acpi_battery_path;
    char info[1024];
    char state[1024];
    char *ptr = NULL;
    char *key = NULL;
    char *val = NULL;
    SDL_bool charge = SDL_FALSE;
    SDL_bool choose = SDL_FALSE;
    SDL_bool is_ac = SDL_FALSE;
    int maximum = -1;
    int remaining = -1;
    int secs = -1;
    int pct = -1;

    if (!load_acpi_file(base, node, "state", state, sizeof (state))) {
        return;
    } else if (!load_acpi_file(base, node, "info", info, sizeof (info))) {
        return;
    }

    ptr = &state[0];
    while (make_proc_acpi_key_val(&ptr, &key, &val)) {
        if (strcmp(key, "present") == 0) {
            if (strcmp(val, "yes") == 0) {
                *have_battery = SDL_TRUE;
            }
        } else if (strcmp(key, "charging state") == 0) {
            /* !!! FIXME: what exactly _does_ charging/discharging mean? */
            if (strcmp(val, "charging/discharging") == 0) {
                charge = SDL_TRUE;
            } else if (strcmp(val, "charging") == 0) {
                charge = SDL_TRUE;
            }
        } else if (strcmp(key, "remaining capacity") == 0) {
            char *endptr = NULL;
            const int cvt = (int) strtol(val, &endptr, 10);
            if (*endptr == ' ') {
                remaining = cvt;
            }
        }
    }

    ptr = &info[0];
    while (make_proc_acpi_key_val(&ptr, &key, &val)) {
        if (strcmp(key, "design capacity") == 0) {
            char *endptr = NULL;
            const int cvt = (int) strtol(val, &endptr, 10);
            if (*endptr == ' ') {
                maximum = cvt;
            }
        }
    }

    if ((maximum >= 0) && (remaining >= 0)) {
        pct = (int) ((((float) remaining) / ((float) maximum)) * 100.0f);
        if (pct < 0) {
            pct = 0;
        } else if (pct > 100) {
            pct = 100;
        }
    }

    /* !!! FIXME: calculate (secs). */

    /*
     * We pick the battery that claims to have the most minutes left.
     *  (failing a report of minutes, we'll take the highest percent.)
     */
    if ((secs < 0) && (*seconds < 0)) {
        if ((pct < 0) && (*percent < 0)) {
            choose = SDL_TRUE;  /* at least we know there's a battery. */
        }
        if (pct > *percent) {
            choose = SDL_TRUE;
        }
    } else if (secs > *seconds) {
        choose = SDL_TRUE;
    }

    if (choose) {
        *seconds = secs;
        *percent = pct;
        *charging = charge;
    }
}

static void
check_proc_acpi_ac_adapter(const char * node, SDL_bool * have_ac)
{
    const char *base = proc_acpi_ac_adapter_path;
    char state[256];
    char *ptr = NULL;
    char *key = NULL;
    char *val = NULL;
    SDL_bool charge = SDL_FALSE;
    SDL_bool choose = SDL_FALSE;
    SDL_bool is_ac = SDL_FALSE;
    int maximum = -1;
    int remaining = -1;
    int secs = -1;
    int pct = -1;

    if (!load_acpi_file(base, node, "state", state, sizeof (state))) {
        return;
    }

    ptr = &state[0];
    while (make_proc_acpi_key_val(&ptr, &key, &val)) {
        if (strcmp(key, "state") == 0) {
            if (strcmp(val, "on-line") == 0) {
                *have_ac = SDL_TRUE;
            }
        }
    }
}


SDL_bool
SDL_GetPowerInfo_Linux_proc_acpi(SDL_PowerState * state,
                                 int *seconds, int *percent)
{
    struct dirent *dent = NULL;
    DIR *dirp = NULL;
    SDL_bool have_battery = SDL_FALSE;
    SDL_bool have_ac = SDL_FALSE;
    SDL_bool charging = SDL_FALSE;

    *seconds = -1;
    *percent = -1;
    *state = SDL_POWERSTATE_UNKNOWN;

    dirp = opendir(proc_acpi_battery_path);
    if (dirp == NULL) {
        return SDL_FALSE;  /* can't use this interface. */
    } else {
        while ((dent = readdir(dirp)) != NULL) {
            const char *node = dent->d_name;
            check_proc_acpi_battery(node, &have_battery, &charging,
                                    seconds, percent);
        }
        closedir(dirp);
    }

    dirp = opendir(proc_acpi_ac_adapter_path);
    if (dirp == NULL) {
        return SDL_FALSE;  /* can't use this interface. */
    } else {
        while ((dent = readdir(dirp)) != NULL) {
            const char *node = dent->d_name;
            check_proc_acpi_ac_adapter(node, &have_ac);
        }
        closedir(dirp);
    }

    if (!have_battery) {
        *state = SDL_POWERSTATE_NO_BATTERY;
    } else if (charging) {
        *state = SDL_POWERSTATE_CHARGING;
    } else if (have_ac) {
        *state = SDL_POWERSTATE_CHARGED;
    } else {
        *state = SDL_POWERSTATE_ON_BATTERY;
    }

    return SDL_TRUE;   /* definitive answer. */
}


static SDL_bool
next_string(char **_ptr, char **_str)
{
    char *ptr = *_ptr;
    char *str = *_str;

    while (*ptr == ' ') {       /* skip any spaces... */
        ptr++;
    }

    if (*ptr == '\0') {
        return SDL_FALSE;
    }

    str = ptr;
    while ((*ptr != ' ') && (*ptr != '\n') && (*ptr != '\0'))
        ptr++;

    if (*ptr != '\0')
        *(ptr++) = '\0';

    *_str = str;
    *_ptr = ptr;
    return SDL_TRUE;
}

static SDL_bool
int_string(char *str, int *val)
{
    char *endptr = NULL;
    *val = (int) strtol(str, &endptr, 0);
    return ((*str != '\0') && (*endptr == '\0'));
}

/* http://lxr.linux.no/linux+v2.6.29/drivers/char/apm-emulation.c */
SDL_bool
SDL_GetPowerInfo_Linux_proc_apm(SDL_PowerState * state,
                                int *seconds, int *percent)
{
    SDL_bool need_details = SDL_FALSE;
    int ac_status = 0;
    int battery_status = 0;
    int battery_flag = 0;
    int battery_percent = 0;
    int battery_time = 0;
    const int fd = open(proc_apm_path, O_RDONLY);
    char buf[128];
    char *ptr = &buf[0];
    char *str = NULL;
    ssize_t br;

    if (fd == -1) {
        return SDL_FALSE;       /* can't use this interface. */
    }

    br = read(fd, buf, sizeof (buf) - 1);
    close(fd);

    if (br < 0) {
        return SDL_FALSE;
    }

    buf[br] = '\0';             // null-terminate the string.
    if (!next_string(&ptr, &str)) {     /* driver version */
        return SDL_FALSE;
    }
    if (!next_string(&ptr, &str)) {     /* BIOS version */
        return SDL_FALSE;
    }
    if (!next_string(&ptr, &str)) {     /* APM flags */
        return SDL_FALSE;
    }

    if (!next_string(&ptr, &str)) {     /* AC line status */
        return SDL_FALSE;
    } else if (!int_string(str, &ac_status)) {
        return SDL_FALSE;
    }

    if (!next_string(&ptr, &str)) {     /* battery status */
        return SDL_FALSE;
    } else if (!int_string(str, &battery_status)) {
        return SDL_FALSE;
    }
    if (!next_string(&ptr, &str)) {     /* battery flag */
        return SDL_FALSE;
    } else if (!int_string(str, &battery_flag)) {
        return SDL_FALSE;
    }
    if (!next_string(&ptr, &str)) {     /* remaining battery life percent */
        return SDL_FALSE;
    }
    if (str[strlen(str) - 1] == '%') {
        str[strlen(str) - 1] = '\0';
    }
    if (!int_string(str, &battery_percent)) {
        return SDL_FALSE;
    }

    if (!next_string(&ptr, &str)) {     /* remaining battery life time */
        return SDL_FALSE;
    } else if (!int_string(str, &battery_time)) {
        return SDL_FALSE;
    }

    if (!next_string(&ptr, &str)) {     /* remaining battery life time units */
        return SDL_FALSE;
    } else if (strcmp(str, "min") == 0) {
        battery_time *= 60;
    }

    if (battery_flag == 0xFF) { /* unknown state */
        *state = SDL_POWERSTATE_UNKNOWN;
    } else if (battery_flag & (1 << 7)) {       /* no battery */
        *state = SDL_POWERSTATE_NO_BATTERY;
    } else if (battery_flag & (1 << 3)) {       /* charging */
        *state = SDL_POWERSTATE_CHARGING;
        need_details = SDL_TRUE;
    } else if (ac_status == 1) {
        *state = SDL_POWERSTATE_CHARGED;        /* on AC, not charging. */
        need_details = SDL_TRUE;
    } else {
        *state = SDL_POWERSTATE_ON_BATTERY;
        need_details = SDL_TRUE;
    }

    *percent = -1;
    *seconds = -1;
    if (need_details) {
        const int pct = battery_percent;
        const int secs = battery_time;

        if (pct >= 0) {         /* -1 == unknown */
            *percent = (pct > 100) ? 100 : pct; /* clamp between 0%, 100% */
        }
        if (secs >= 0) {        /* -1 == unknown */
            *seconds = secs;
        }
    }

    return SDL_TRUE;
}

#endif /* SDL_POWER_LINUX */
#endif /* SDL_POWER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
