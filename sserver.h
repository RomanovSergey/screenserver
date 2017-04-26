#ifndef SSERVER_H
#define SSERVER_H

#include <QCoreApplication>
#include <QObject>
#include <QUdpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QImage>
#include <QBuffer>
#include <QString>
#include <QStringList>
#include <iostream>
#include <csignal>

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>

class Sserver : public QObject
{
    Q_OBJECT
private:
    int         m_portL; //port for listen
    QString     m_pathF; //path to frame buffer
    QByteArray *m_b;
    QUdpSocket *m_udp;

    static int m_fb_fd;
    static struct fb_var_screeninfo  m_vscr;
    static struct fb_fix_screeninfo  m_fscr;

    static bool openFB(const QString &pathFB);
    static void exitQt(int sig);

public:
    Sserver(int, QString);
    ~Sserver();

    int getPort();
    bool getImg();

    static void savePNG(const QString& pathFB, const QString& );
    static void printFBinfo(const QString& pathFB);

private slots:
    void slotReadDatagrams();
};

#endif // SSERVER_H
