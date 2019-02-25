#include <iostream>
#include <MediaInfoDLL/MediaInfoDLL.h>

#define MediaInfoNameSpace MediaInfoDLL;

using namespace std;
using namespace MediaInfoNameSpace;

int main(int argc, char **argv) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " musicfile" << endl;    
        return 1;
    }
    else {
        MediaInfo MI;
        MI.Option(__T("Internet"), __T("No"));
        MI.Open(__T(argv[1]));
        if (MI.Get(Stream_General, 0, "Cover") == "Yes") {
            cout << MI.Get(Stream_General, 0, "Cover_Mime") << endl;
            cout << MI.Get(Stream_General, 0, "Cover_Data");
        }
    }
    return 0;
}
