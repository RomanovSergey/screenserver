/*
 * Autor: Romanov Se
 *   01.01.2017
 */

#include "sserver.h"
#include <QStringList>
#include <QFile>


using std::cout;
using std::cerr;
using std::endl;


int    Sserver::m_fb_fd;
struct fb_var_screeninfo  Sserver::m_vscr;
struct fb_fix_screeninfo  Sserver::m_fscr;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QStringList cmdline = a.arguments();
    QString str;
    QString path;
    bool ok;
    unsigned int port;

    port = 50000;      //default port for lister
    path = "/dev/fb0"; //default path to frame buffer device

    int max = cmdline.size();
    for(int i=1; i<max; i++) {
        str = cmdline.at(i);
        if ( (str == "--help") || (str == "-h") ){//=============== HELP ==============
            cout << "Usage:" << endl;
            cout << "  -h              Shows this message, and exit." << endl;
            cout << "  -p <uint16_t>   Set listening udp port (default " << port << ")." << endl;
            cout << "  -f <path>       Set path to FB device (default " << path.toStdString() << ")." << endl;
            cout << "  -i              Print frame buffer info, and exit." << endl;
            cout << "  -s <file.png>   Save FB image to file in PNG format, and exit" << endl;
            return 0;
        }else if (str == "-p"){//=============== Set listening port ==============
            i++;
            if (i < max){
                str = cmdline.at(i);
                port = str.toUInt(&ok);
                if (ok == false) {
                    cerr << "error: expect value in -p param, see help (-h)" << endl;
                    return -1;
                }
                if (port > 65535) {
                    cerr << "error: port number more then uint16_t" << endl;
                    return -1;
                }
            }else{
                cerr << "error: absence argument in -p param, see help (-h)" << endl;
                return -1;
            }
        }else if (str == "-f") {
            i++;
            if (i < max){
                path = cmdline.at(i);
                QFile file(path);
                if (!file.exists()){
                    cerr << "file: " << path.toStdString() << " does not exist." << endl;
                    return -1;
                }
            }else{
                cerr << "error: absence argument in -f param, see help (-h)" << endl;
                return -1;
            }
        }else if (str == "-s") {
            i++;
            if (i < max){
                Sserver::savePNG(path, cmdline.at(i));
                return 0;
            }else{
                cerr << "error: absence argument in -s param, see help (-h)" << endl;
                return -1;
            }
        }else if (str == "-i"){
            Sserver::printFBinfo(path);
            return 0;
        }else{//=============== UNKNOWN ==============
            cerr << "unknown param: " << str.toStdString() << endl;
            return 0;
        }
    }

    Sserver s(port, path);

    cout << "listen udp port=" << s.getPort() << " for query on image send to client" << endl;
    cout << "frame buffer path: " << path.toStdString() << endl << endl;

    return a.exec();
}
