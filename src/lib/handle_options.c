/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2022 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

#include "compile_time.h"
#include "handle_options.h"

#include "config_def.h"
#include "pin.h"
#include "sds_extras.h"

#include <getopt.h>
#include <stdio.h>

static struct option long_options[] = {
    {"cachedir",  required_argument, 0, 'a'},
    {"config",    no_argument,       0, 'c'},
    {"help",      no_argument,       0, 'h'},
    #ifdef ENABLE_SSL
    {"pin",       no_argument,       0, 'p'},
    #endif
    {"syslog",    no_argument,       0, 's'},
    {"user",      required_argument, 0, 'u'},
    {"version",   no_argument,       0, 'v'},
    {"workdir",   required_argument, 0, 'w'}
};

/**
 * Prints the command line usage information
 * @param config pointer to config struct
 * @param cmd argv[0] from main function
 */
static void print_usage(struct t_config *config, const char *cmd) {
    fprintf(stderr, "\nUsage: %s [OPTION]...\n\n"
                    "myMPD %s\n"
                    "(c) 2018-2022 Juergen Mang <mail@jcgames.de>\n"
                    "https://github.com/jcorporation/myMPD\n\n"
                    "Options:\n"
                    "  -c, --config           creates config and exits\n"
                    "  -h, --help             displays this help\n"
                    "  -v, --version          displays this help\n"
                    "  -u, --user <username>  username to drop privileges to (default: mympd)\n"
                    "  -s, --syslog           enable syslog logging (facility: daemon)\n"
                    "  -w, --workdir <path>   working directory (default: %s)\n"
                    "  -a, --cachedir <path>  cache directory (default: %s)\n",
        cmd, MYMPD_VERSION, config->workdir, config->cachedir);
    #ifdef ENABLE_SSL
    fprintf(stderr, "  -p, --pin              sets a pin for myMPD settings\n");
    #endif
    fprintf(stderr, "\n");
}

/**
 * Handles the command line arguments
 * @param config pointer to myMPD static configuration
 * @param argc from main function
 * @param argv from main function
 * @return OPTIONS_RC_INVALID on error
 *         OPTIONS_RC_EXIT if myMPD should exit
 *         OPTIONS_RC_OK if arguments are parsed successfully
 */
int handle_options(struct t_config *config, int argc, char **argv) {
    int n = 0;
    int option_index = 0;
    while((n = getopt_long(argc, argv, "hu:sw:cpa:v", long_options, &option_index)) != -1) { /* Flawfinder: ignore */
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
            case 'p': {
                bool rc = pin_set(config->workdir);
                return rc == true ? OPTIONS_RC_EXIT : OPTIONS_RC_INVALID;
                break;
            }
            #endif
            case 'v':
            case 'h':
                print_usage(config, argv[0]);
                return OPTIONS_RC_EXIT;
            default:
                print_usage(config, argv[0]);
                return OPTIONS_RC_INVALID;
        }
    }
    return OPTIONS_RC_OK;
}
