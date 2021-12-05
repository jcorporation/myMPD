/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2021 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "mympd_config_defs.h"
#include "handle_options.h"

#include "lib/mympd_pin.h"
#include "lib/sds_extras.h"

#include <getopt.h>
#include <stdio.h>

static struct option long_options[] = {
    #ifdef ENABLE_SSL
    {"pin",       no_argument,       0, 'p'},
    #endif
    {"help",      no_argument,       0, 'h'},
    {"user",      required_argument, 0, 'u'},
    {"syslog",    no_argument,       0, 's'},
    {"workdir",   required_argument, 0, 'w'},
    {"config",    no_argument,       0, 'c'},
    {"cachedir",  required_argument, 0, 'a'}
};

bool handle_options(struct t_config *config, int argc, char **argv) {
    int n = 0;
    int option_index = 0;
    while((n = getopt_long(argc, argv, "hu:sw:cpa:", long_options, &option_index)) != -1) { /* Flawfinder: ignore */
        switch (n) {
            case 'u':
                config->user = sds_replace(config->user, optarg);
                break;
            case 's':
                config->log_to_syslog = true;
                break;
            case 'w':
                config->workdir = sds_replace(config->workdir, optarg);
                break;
            case 'a':
                config->cachedir = sds_replace(config->cachedir, optarg);
                break;
            case 'c':
                config->bootstrap = true;
                break;
            #ifdef ENABLE_SSL
            case 'p':
                pin_set(config->workdir);
                return false;
                break;
            #endif
            default:
                fprintf(stderr, "\nUsage: %s [OPTION]...\n\n"
                                "myMPD %s\n"
                                "(c) 2018-2021 Juergen Mang <mail@jcgames.de>\n"
                                "https://github.com/jcorporation/myMPD\n\n"
                                "Options:\n"
                                "  -c, --config           creates config and exits\n"
                                "  -h, --help             displays this help\n"
                                "  -u, --user <username>  username to drop privileges to (default: mympd)\n"
                                "  -s, --syslog           enable syslog logging (facility: daemon)\n"
                                "  -w, --workdir <path>   working directory (default: %s)\n"
                                "  -a, --cachedir <path>  cache directory (default: %s)\n",
                    argv[0], MYMPD_VERSION, config->workdir, config->cachedir);
                #ifdef ENABLE_SSL
                fprintf(stderr, "  -p, --pin              sets a pin for myMPD settings\n");
                #endif
                fprintf(stderr, "\n");
                return false;
        }
    }
    return true;
}
