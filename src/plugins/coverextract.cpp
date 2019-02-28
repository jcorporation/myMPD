/* myMPD
   (c) 2018-2019 Juergen Mang <mail@jcgames.de>
   This project's homepage is: https://github.com/jcorporation/mympd
   
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

#include <iostream>
#include <fstream>
#include <string.h>

#include "coverextract.h"

#include <MediaInfoDLL/MediaInfoDLL.h>
#define MediaInfoNameSpace MediaInfoDLL;

using namespace std;
using namespace MediaInfoNameSpace;

static const std::string base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static inline bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

static string base64_decode(std::string const& encoded_string) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    string ret;

    while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i ==4) {
            for (i = 0; i <4; i++) {
                char_array_4[i] = base64_chars.find(char_array_4[i]);
            }
            char_array_3[0] = ( char_array_4[0] << 2       ) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) +   char_array_4[3];

            for (i = 0; (i < 3); i++) {
                ret += char_array_3[i];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = 0; j < i; j++) {
            char_array_4[j] = base64_chars.find(char_array_4[j]);
	}

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; (j < i - 1); j++) {
            ret += char_array_3[j];
        }
    }

    return ret;
}

int coverextract(const char *media_file_ptr, char *buffer, int buffer_len, const bool extract) {
    string media_file = media_file_ptr;
    MediaInfo MI;
    MI.Option(__T("Internet"), __T("No"));
    MI.Open(__T(media_file));
    if (MI.Get(Stream_General, 0, "Cover") == "Yes") {
        string mime_type = MI.Get(Stream_General, 0, "Cover_Mime");
        string ext = mime_type.substr(mime_type.find_last_of("/") + 1);
        string output_file = media_file.substr(0, media_file.find_last_of(".")) + "." + ext;
        for (int i = 0 ; i <= output_file.size(); i++) {
            if (output_file[i] == '/') {
                output_file[i] = '_';
            }
        }
        if (extract == true) {
//TODO: test if file exists
            ofstream myfile;
            myfile.open("/var/lib/mympd/covercache/" + output_file);
            if (myfile.is_open()) {
                myfile << base64_decode(MI.Get(Stream_General, 0, "Cover_Data"));
                myfile.close();
            }
            else {
                strncpy(buffer, "cantwrite", buffer_len);
		MI.Close();
                return 1;
            }
        }
        strncpy(buffer, output_file.c_str(), buffer_len);
    }
    else {
        strncpy(buffer, "nocover", buffer_len);
	MI.Close();
        return 1;
    }
    MI.Close();
    return 0;
}
