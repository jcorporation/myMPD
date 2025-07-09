/*
 SPDX-License-Identifier: GPL-3.0-or-later
 myMPD (c) 2018-2025 Juergen Mang <mail@jcgames.de>
 https://github.com/jcorporation/mympd
*/

/*! \file
 * \brief Command line options handling
 */

#include "compile_time.h"
#include "src/lib/handle_options.h"

#include "src/lib/config.h"
#include "src/lib/config_def.h"
#include "src/lib/pin.h"
#include "src/lib/sds_extras.h"
#include "src/lib/utility.h"

#include <getopt.h>

/**
 * Options definitions
 */
static struct option long_options[] = {
    {"cachedir",  required_argument, 0, 'a'},
    {"config",    no_argument,       0, 'c'},
    {"dump",      no_argument,       0, 'd'},
    {"help",      no_argument,       0, 'h'},
    {"pin",       no_argument,       0, 'p'},
    {"syslog",    no_argument,       0, 's'},
    {"version",   no_argument,       0, 'v'},
    {"workdir",   required_argument, 0, 'w'}
};

/**
 * Prints the command line usage information
 * @param config pointer to config struct
 * @param cmd argv[0] from main function
 */
static void print_usage(struct t_config *config, const char *cmd) {
    printf("\nUsage: %s [OPTION]...\n\n"
                    "myMPD %s\n"
                    "(c) 2018-2025 Juergen Mang <mail@jcgames.de>\n"
                    "https://github.com/jcorporation/myMPD\n\n"
                    "Options:\n"
                    "  -c, --config           Creates config and exits\n"
                    "  -d, --dump             Dumps default config and exits\n"
                    "  -s, --syslog           Enable syslog logging (facility: daemon)\n"
                    "  -w, --workdir <path>   Working directory (default: %s)\n"
                    "  -a, --cachedir <path>  Cache directory (default: %s)\n"
                    "  -h, --help             Displays this help\n"
                    "  -v, --version          Displays this help\n"
                    "  -p, --pin              Sets a pin for myMPD settings\n\n",
        cmd, MYMPD_VERSION, config->workdir, config->cachedir);
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
enum handle_options_rc handle_options(struct t_config *config, int argc, char **argv) {
    int n = 0;
    int option_index = 0;
    while ((n = getopt_long(argc, argv, "a:cdhpsvw:", long_options, &option_index)) != -1) {
        switch(n) {
            case 'a':
                config->cachedir = sds_replace(config->cachedir, optarg);
                strip_slash(config->cachedir);
                break;
            case 'c':
                config->bootstrap = true;
                break;
            case 'd':
                mympd_config_dump_default();
                return OPTIONS_RC_EXIT;
            case 'p': {
                bool rc = pin_set(config->workdir);
                return rc == true ? OPTIONS_RC_EXIT : OPTIONS_RC_INVALID;
            }
            case 's':
                config->log_to_syslog = true;
                break;
            case 'v':
            case 'h':
                print_usage(config, argv[0]);
                return OPTIONS_RC_EXIT;
            case 'w':
                config->workdir = sds_replace(config->workdir, optarg);
                strip_slash(config->workdir);
                break;
            default:
                print_usage(config, argv[0]);
                return OPTIONS_RC_INVALID;
        }
    }
    return OPTIONS_RC_OK;
}
