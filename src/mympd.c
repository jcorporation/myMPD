/* myMPD
   (c) 2018 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/ympd
   
   myMPD ist fork of:
   
   ympd
   (c) 2013-2014 Andrew Karpow <andy@ndyk.de>
   This project's homepage is: http://www.ympd.org
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <pwd.h>

#include "mongoose/mongoose.h"
#include "mpd_client.h"
#include "config.h"

extern char *optarg;
static sig_atomic_t s_signal_received = 0;
static struct mg_serve_http_opts s_http_server_opts;

static void signal_handler(int sig_num) {
  signal(sig_num, signal_handler);  // Reinstantiate signal handler
  s_signal_received = sig_num;
}

static void handle_api(struct mg_connection *nc, struct http_message *hm) {
  mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
  char buf[1000] = {0};
  memcpy(buf, hm->body.p,sizeof(buf) - 1 < hm->body.len ? sizeof(buf) - 1 : hm->body.len);
  struct mg_str d = {buf, strlen(buf)};
  callback_mpd(nc, d);
  mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
}

static void ev_handler(struct mg_connection *nc, int ev, void *ev_data) {
    switch(ev) {
        case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
             #ifdef DEBUG
             fprintf(stdout,"New Websocket connection\n");
             #endif
             struct mg_str d = {(char *) "MPD_API_WELCOME" };
             callback_mpd(nc, d);
             break;
        }
        case MG_EV_WEBSOCKET_FRAME: {
            struct websocket_message *wm = (struct websocket_message *) ev_data;
            wm->data[wm->size]='\0';
            struct mg_str d = {(char *) wm->data, wm->size};
            #ifdef DEBUG
            fprintf(stdout,"Websocket request: %s\n",wm->data);
            #endif
            callback_mpd(nc, d);
            break;
        }
        case MG_EV_HTTP_REQUEST: {
            struct http_message *hm = (struct http_message *) ev_data;
            #ifdef DEBUG
            printf("HTTP request: %.*s\n",hm->uri.len,hm->uri.p);
            #endif
            if (mg_vcmp(&hm->uri, "/api") == 0) {
              handle_api(nc, hm);

            }
            else {
              mg_serve_http(nc, hm, s_http_server_opts);
            }
            break;
        }
        case MG_EV_CLOSE: {
            if (is_websocket(nc)) {
              #ifdef DEBUG
              fprintf(stdout,"Websocket connection closed\n");
              #endif
              mpd_close_handler(nc);
            }
            else {
              #ifdef DEBUG
              fprintf(stdout,"HTTP Close\n");
              #endif
            }
            break;
        }        
    }
}

int main(int argc, char **argv)
{
    int n, option_index = 0;
    struct mg_mgr mgr;
    struct mg_connection *nc;
    unsigned int current_timer = 0, last_timer = 0;
    char *run_as_user = NULL;
    char *webport = "8080";
    mpd.port = 6600;
    mpd.local_port = 0;
    mpd.gpass = NULL;
    strcpy(mpd.host, "127.0.0.1");
    streamport = 8000;
    strcpy(coverimage, "folder.jpg");

    static struct option long_options[] = {
        {"host",         required_argument, 0, 'h'},
        {"port",         required_argument, 0, 'p'},
        {"webport",      required_argument, 0, 'w'},
        {"user",         required_argument, 0, 'u'},
        {"version",      no_argument,       0, 'v'},
        {"help",         no_argument,       0,  0 },
        {"mpdpass",      required_argument, 0, 'm'},
        {"streamport",	 required_argument, 0, 's'},
        {"coverimage",	 required_argument, 0, 'i'},
        {0,              0,                 0,  0 }
    };

    while((n = getopt_long(argc, argv, "D:h:p:l:w:u:d:vm:s:i:",
                long_options, &option_index)) != -1) {
        switch (n) {
            case 'D':
                mpd.gpass = strdup(optarg);
                break;
            case 'h':
                strncpy(mpd.host, optarg, sizeof(mpd.host));
                break;
            case 'p':
                mpd.port = atoi(optarg);
                break;
            case 'w':
                webport = strdup(optarg);
                break;
            case 'u':
                run_as_user = strdup(optarg);
                break;
            case 'm':
                if (strlen(optarg) > 0)
                    mpd.password = strdup(optarg);
                break;
            case 's':
                streamport = atoi(optarg);
                break;
            case 'i':
                strncpy(coverimage, optarg, sizeof(coverimage));
                break;
            case 'v':
                fprintf(stdout, "myMPD  %d.%d.%d\n"
                        "Copyright (C) 2018 Juergen Mang <mail@jcgames.de>\n"
                        "Built " __DATE__ " "__TIME__ "\n",
                        MYMPD_VERSION_MAJOR, MYMPD_VERSION_MINOR, MYMPD_VERSION_PATCH);
                return EXIT_SUCCESS;
                break;
            default:
                fprintf(stderr, "Usage: %s [OPTION]...\n\n"
                        " -h, --host <host>\t\tconnect to mpd at host [localhost]\n"
                        " -p, --port <port>\t\tconnect to mpd at port [6600]\n"
                        " -w, --webport [ip:]<port>\tlisten interface/port for webserver [8080]\n"
                        " -u, --user <username>\t\tdrop priviliges to user after socket bind\n"
                        " -v, --version\t\t\tget version\n"
                        " -m, --mpdpass <password>\tspecifies the password to use when connecting to mpd\n"
                        " -s, --streamport <port>\tconnect to mpd http stream at port [8000]\n"
                        " -i, --coverimage <filename>\tfilename for coverimage [folder.jpg]\n"
                        " --help\t\t\t\tthis help\n"
                        , argv[0]);
                return EXIT_FAILURE;
        }

    }

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    setvbuf(stdout, NULL, _IOLBF, 0);
    setvbuf(stderr, NULL, _IOLBF, 0);
    
    mg_mgr_init(&mgr, NULL);

    nc = mg_bind(&mgr, webport, ev_handler);
    if (nc == NULL) {
       printf("myMPD can't bind to port %s\n", webport);
       return EXIT_FAILURE;
    }

    if(run_as_user != NULL) {
        printf("Droping privileges\n");
        struct passwd *pw;
        if ((pw = getpwnam(run_as_user)) == NULL) {
            printf("Unknown user\n");
            return EXIT_FAILURE;
        } else if (setgid(pw->pw_gid) != 0) {
            printf("setgid() failed");
            return EXIT_FAILURE;
        } else if (setuid(pw->pw_uid) != 0) {
            printf("setuid() failed\n");
            return EXIT_FAILURE;
        }
        free(run_as_user);
    }

    mg_set_protocol_http_websocket(nc);
    s_http_server_opts.document_root = SRC_PATH;

    printf("myMPD started on port %s\n", webport);
    while (s_signal_received == 0) {
        mg_mgr_poll(&mgr, 200);
        current_timer = time(NULL);
        if(current_timer - last_timer)
        {
            last_timer = current_timer;
            mpd_poll(&mgr);
        }
    }
    mg_mgr_free(&mgr);
    mpd_disconnect();
    return EXIT_SUCCESS;
}
