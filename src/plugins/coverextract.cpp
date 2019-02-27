#include <iostream>
#include <fstream>

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
        for (j = 0; j < i; j++)
        char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; (j < i - 1); j++) {
            ret += char_array_3[j];
        }
    }

    return ret;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " musicfile" << endl;    
        return 1;
    }
    else {
        string media_file = argv[1];
        MediaInfo MI;
        MI.Option(__T("Internet"), __T("No"));
        MI.Open(__T(media_file));
        if (MI.Get(Stream_General, 0, "Cover") == "Yes") {
            string mime_type = MI.Get(Stream_General, 0, "Cover_Mime");
            string ext = mime_type.substr(mime_type.find_last_of("/") + 1);
            string output_file = media_file.substr(0, media_file.find_last_of(".")) + "." + ext;
            for(int i = 0 ; i <= output_file.size(); i++) {
                if (output_file[i] == '/') {
                    output_file[i]='_';
                }
            }
            ofstream myfile;
            myfile.open(output_file);
            if (myfile.is_open()) {
                myfile << base64_decode(MI.Get(Stream_General, 0, "Cover_Data"));
                myfile.close();
                cout << output_file << endl;
            }
            else {
                cout << "Error opening file " << output_file << endl;
                return 1;
            }
        }
        else {
            cout << "No cover found in file " << media_file << endl;
            return 1;
        }
    }
    return 0;
}
