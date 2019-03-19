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
#include <sys/stat.h>

#include "mympd_coverextract.h"

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

static bool write_base64_decoded(std::string const& encoded_string, const std::string& abs_tmp_file) {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    
    ofstream myfile;
    myfile.open(abs_tmp_file);
    if (!myfile.is_open()) {
        return false;
    }

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
                myfile << char_array_3[i];
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
            myfile << char_array_3[j];
        }
    }

    myfile.close();
    return true;
}

static bool file_exists(const std::string& file) {
    struct stat buf;
    return (stat(file.c_str(), &buf) == 0);
}

int coverextract(const char *media_file_ptr, char *image_filename, int image_filename_len, char *image_mime_type, int image_mime_type_len, const bool extract) {
    string media_file = media_file_ptr;
    MediaInfo MI;
    MI.Option(__T("Internet"), __T("No"));
    MI.Option(__T("Cover_Data"), __T("base64"));
    MI.Open(__T(media_file));
    if (MI.Get(Stream_General, 0, "Cover") == "Yes") {
        string mime_type = MI.Get(Stream_General, 0, "Cover_Mime");
        strncpy(image_mime_type, mime_type.c_str(), image_mime_type_len);
        string ext = mime_type.substr(mime_type.find_last_of("/") + 1);
        string output_file = media_file.substr(0, media_file.find_last_of(".")) + "." + ext;
        for (int i = 0 ; i < output_file.size(); i++) {
            if (output_file[i] == '/') {
                output_file[i] = '_';
            }
        }
        if (extract == true) {
            string abs_output_file = "/var/lib/mympd/covercache/" + output_file;
            string abs_tmp_file = "/var/lib/mympd/covercache/" + output_file + ".tmp";
            if (file_exists(abs_output_file) == false) {
                bool rc = write_base64_decoded(MI.Get(Stream_General, 0, "Cover_Data"), abs_tmp_file);
                if (rc == true) {
                    rename(abs_tmp_file.c_str(), abs_output_file.c_str());
                }
                else {
                    strncpy(image_filename, "cantwrite", image_filename_len);
                    strncpy(image_mime_type, "nomimetype", image_mime_type_len);
                    MI.Close();
                    return 1;
                }
            }
        }
        strncpy(image_filename, output_file.c_str(), image_filename_len);
    }
    else {
        strncpy(image_filename, "nocover", image_filename_len);
        strncpy(image_mime_type, "nomimetype", image_mime_type_len);
	MI.Close();
        return 1;
    }
    MI.Close();
    return 0;
}
