/*
 SPDX-License-Identifier: GPL-2.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>

#include "dist/src/sds/sds.h"
#include "dist/src/rax/rax.h"
#include "list.h"
#include "sds_extras.h"
#include "mympd_config_defs.h"


static struct option long_options[] = {
    {"help",      no_argument,       0, 'h'},
    {"user",      required_argument, 0, 'u'},
    {"syslog",    no_argument,       0, 's'},
    {"workdir",   required_argument, 0, 'w'}
};
   
bool handle_options(struct t_config *config, int argc, char **argv) {
    int n = 0;
    int option_index = 0;
    while((n = getopt_long(argc, argv, "hu:sw:", long_options, &option_index)) != -1) { /* Flawfinder: ignore */
        switch (n) {
            case 'u':
                config->user = sdsreplace(config->user, optarg);
                break;
            case 's':
                config->syslog = true;
                break;
            case 'w':
                config->workdir = sdsreplace(config->workdir, optarg);
                break;
            default:
                fprintf(stderr, "\nUsage: %s [OPTION]...\n\n"
                    "myMPD %s\n"
                    "(c) 2018-2021 Juergen Mang <mail@jcgames.de>\n"
                    "https://github.com/jcorporation/myMPD\n\n"
                    "Options:\n"
                    "  -h, --help             displays this help\n"
                    "  -u, --user <username>  username to drop privileges to (default: mympd)\n"
                    "  -s, --syslog           enable syslog logging (facility: daemon)\n"
                    "  -w, --workdir <path>   working directory (default: %s)\n\n",
                    argv[0], MYMPD_VERSION, config->workdir);
                return false;
        
        }
    }
    return true;
}
