// vim:ts=8:expandtab
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "i3status.h"

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

/*
 * Get battery information from /sys. Note that it uses the design capacity to
 * calculate the percentage, not the last full capacity, so you can see how
 * worn off your battery is.
 *
 */
const char *get_battery_info(struct battery *bat) {
        char buf[1024];
        static char part[512];
        char *walk, *last;
        int full_design = -1,
            remaining = -1,
            present_rate = -1;
        charging_status_t status = CS_DISCHARGING;

#if defined(LINUX)
        if (!slurp(bat->path, buf, sizeof(buf)))
                return "No battery";

        for (walk = buf, last = buf; (walk-buf) < 1024; walk++) {
                if (*walk == '\n') {
                        last = walk+1;
                        continue;
                }

                if (*walk != '=')
                        continue;

                if (BEGINS_WITH(last, "POWER_SUPPLY_ENERGY_NOW") ||
                    BEGINS_WITH(last, "POWER_SUPPLY_CHARGE_NOW"))
                        remaining = atoi(walk+1);
                else if (BEGINS_WITH(last, "POWER_SUPPLY_CURRENT_NOW"))
                        present_rate = atoi(walk+1);
                else if (BEGINS_WITH(last, "POWER_SUPPLY_STATUS=Charging"))
                        status = CS_CHARGING;
                else if (BEGINS_WITH(last, "POWER_SUPPLY_STATUS=Full"))
                        status = CS_FULL;
                else {
                        /* The only thing left is the full capacity */
                        if (bat->use_last_full) {
                                if (!BEGINS_WITH(last, "POWER_SUPPLY_ENERGY_FULL") &&
                                    !BEGINS_WITH(last, "POWER_SUPPLY_CHARGE_FULL"))
                                        continue;
                        } else {
                                if (!BEGINS_WITH(last, "POWER_SUPPLY_CHARGE_FULL_DESIGN") &&
                                    !BEGINS_WITH(last, "POWER_SUPPLY_ENERGY_FULL_DESIGN"))
                                        continue;
                        }

                        full_design = atoi(walk+1);
                }
        }

        if ((full_design == 1) || (remaining == -1))
                return part;

        if (present_rate > 0) {
                float remaining_time;
                int seconds, hours, minutes;
                if (status == CS_CHARGING)
                        remaining_time = ((float)full_design - (float)remaining) / (float)present_rate;
                else if (status == CS_DISCHARGING)
                        remaining_time = ((float)remaining / (float)present_rate);
                else remaining_time = 0;

                seconds = (int)(remaining_time * 3600.0);
                hours = seconds / 3600;
                seconds -= (hours * 3600);
                minutes = seconds / 60;
                seconds -= (minutes * 60);

                (void)snprintf(part, sizeof(part), "%s %.02f%% %02d:%02d:%02d",
                        (status == CS_CHARGING ? "CHR" :
                         (status == CS_DISCHARGING ? "BAT" : "FULL")),
                        (((float)remaining / (float)full_design) * 100),
                        max(hours, 0), max(minutes, 0), max(seconds, 0));
        } else {
                (void)snprintf(part, sizeof(part), "%s %.02f%%",
                        (status == CS_CHARGING ? "CHR" :
                         (status == CS_DISCHARGING ? "BAT" : "FULL")),
                        (((float)remaining / (float)full_design) * 100));
        }
#elif defined(__FreeBSD__) || defined(__FreeBSD_kernel__)
        int state;
        int sysctl_rslt;
        size_t sysctl_size = sizeof(sysctl_rslt);

        if (sysctlbyname(BATT_LIFE, &sysctl_rslt, &sysctl_size, NULL, 0) != 0)
                return "No battery";

        present_rate = sysctl_rslt;
        if (sysctlbyname(BATT_TIME, &sysctl_rslt, &sysctl_size, NULL, 0) != 0)
                return "No battery";

        remaining = sysctl_rslt;
        if (sysctlbyname(BATT_STATE, &sysctl_rslt, &sysctl_size, NULL,0) != 0)
                return "No battery";

        state = sysctl_rslt;
        if (state == 0 && present_rate == 100)
                status = CS_FULL;
        else if (state == 0 && present_rate < 100)
                status = CS_CHARGING;
        else
                status = CS_DISCHARGING;

        full_design = sysctl_rslt;

        if (state == 1) {
                int hours, minutes;
                minutes = remaining;
                hours = minutes / 60;
                minutes -= (hours * 60);
                (void)snprintf(part, sizeof(part), "%s %02d%% %02dh%02d",
                               (status == CS_CHARGING ? "CHR" :
                                (status == CS_DISCHARGING ? "BAT" : "FULL")),
                               present_rate,
                               max(hours, 0), max(minutes, 0));
        } else {
                (void)snprintf(part, sizeof(part), "%s %02d%%",
                               (status == CS_CHARGING ? "CHR" :
                                (status == CS_DISCHARGING ? "BAT" : "FULL")),
                               present_rate);
        }
#endif
        return part;
}
