#include <stdio.h>
#include <string.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_version.h>
#include "i3status.h"

void print_run_watch(yajl_gen json_gen, char *buffer, const char *title, const char *pidfile, const char *format) {
	bool running = process_runs(pidfile);
	const char *walk;
	char *outwalk = buffer;

	INSTANCE(pidfile);

	START_COLOR((running ? "color_good" : "color_bad"));

        for (walk = format; *walk != '\0'; walk++) {
                if (*walk != '%') {
			*(outwalk++) = *walk;
                        continue;
                }

                if (strncmp(walk+1, "title", strlen("title")) == 0) {
			outwalk += sprintf(outwalk, "%s", title);
                        walk += strlen("title");
                } else if (strncmp(walk+1, "status", strlen("status")) == 0) {
			outwalk += sprintf(outwalk, "%s", (running ? "yes" : "no"));
                        walk += strlen("status");
                }
        }

	END_COLOR;
	OUTPUT_FULL_TEXT(buffer);
}
