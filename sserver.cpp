#include "sserver.h"

using std::cout;
using std::cerr;
using std::endl;

Sserver::Sserver(int p, QString path) : QObject()
{
    m_portL = p;
    m_pathF = path;

    signal(SIGINT, &Sserver::exitQt);
    signal(SIGTERM, &Sserver::exitQt);

    m_b = new QByteArray;
    m_udp = new QUdpSocket(this);
    m_udp->bind((quint16)m_portL);

    connect(m_udp, SIGNAL(readyRead()), SLOT(slotReadDatagrams()));
}

Sserver::~Sserver()
{
    delete m_udp;
    delete m_b;
}

int Sserver::getPort()
{
    return m_portL;
}

bool Sserver::openFB(const QString &pathFB)
{
    m_fb_fd = open( pathFB.toAscii(), O_RDONLY );
    if ( !m_fb_fd ) {
        cerr << "Could not open framebuffer" << endl;
        return false;
    }
    if ( ioctl( m_fb_fd, FBIOGET_VSCREENINFO, &m_vscr ) ) {
        cerr << "error: Could not FBIOGET_VSCREENINFO" << endl;
        return false;
    }
    if ( ioctl ( m_fb_fd, FBIOGET_FSCREENINFO, &m_fscr ) ) {
        cerr << "error: Could not FBIOGET_FSCREENINFO" << endl;
        return false;
    }
    return true;
}

void Sserver::slotReadDatagrams()
{
    QByteArray ba;//for convert read data to string

    do{
        ba.resize(m_udp->pendingDatagramSize());
        m_udp->readDatagram(ba.data(), ba.size());
    }while(m_udp->hasPendingDatagrams());//обработаем только последную датаграмму

    if (ba.size() > 60){
        cerr << "error: recieve too long datagramm" << endl;
        return;
    }

    QString str;
    str.append(ba);
    QStringList lst;
    lst = str.split(" ");

    cout << endl << str.toStdString() << endl;

    if (lst.size() != 4) {
        cerr << "get items not equal to 4 (port <port> ip <ip>)" << endl;
        return;
    }
    if (lst.at(0) != "port") {
        cerr << "error: first item must be port string" << endl;
        return;
    }
    bool ok;
    int port = lst.at(1).toUInt(&ok);
    if (ok == false) {
        cerr << "error: can not convert port number to int" << endl;
        return;
    }
    if (port > 65535) {
        cerr << "error: port number > 65535" << endl;
        return;
    }
    if (lst.at(2) != "ip") {
        cerr << "error: third item must be ip string" << endl;
        return;
    }
    QHostAddress ha;
    if ( !ha.setAddress(lst.at(3)) ) {
        cerr << "error: can not convert ip address" << endl;
        return;
    }

    if (!getImg()) {
        cerr << "error: Can't get image" << endl;
        return;
    }
    m_udp->writeDatagram(*m_b, ha, port);
}

bool Sserver::getImg()
{
    if ( !openFB( m_pathF ) ) {
        cerr << "Frame Buffer do not opens" << endl;
        close( m_fb_fd );
        return false;
    }

    int buffer_size = m_fscr.line_length * m_vscr.yres_virtual;

    // map the device to memory
    uint32_t *fbp = (uint32_t *)mmap(0, buffer_size, PROT_READ,
                                     MAP_PRIVATE, m_fb_fd, m_vscr.xoffset * m_fscr.line_length);
    if ((long)fbp == -1) {
        cerr << "error: Error: failed to map framebuffer device to memory." << endl;
        return false;
    }

    QBuffer buffer(m_b);
    buffer.open(QIODevice::WriteOnly);

    QImage *img = new QImage((const uchar *)fbp, m_vscr.xres, m_vscr.yres, m_fscr.line_length,
         (m_vscr.bits_per_pixel == 16)?(QImage::QImage::Format_RGB16):(QImage::QImage::Format_RGB32));

    img->save(&buffer, "PNG"); // writes image into ba in PNG format
    delete img;

    buffer.close();
    int ret = munmap(fbp, buffer_size);
    if (ret != 0) {
        cerr << "err: munmap return not zero: " << ret << endl;
    }
    close( m_fb_fd);
    return true;
}

void Sserver::savePNG(const QString &pathFB, const QString &name)
{
    if ( !openFB( pathFB ) ) {
        cerr << "Frame Buffer do not opens" << endl;
        close( m_fb_fd );
        return;
    }

    int buffer_size = m_fscr.line_length * m_vscr.yres_virtual;

    // map the device to memory
    uint32_t *fbp = (uint32_t *)mmap(0, buffer_size, PROT_READ,
                                     MAP_PRIVATE, m_fb_fd, m_vscr.xoffset * m_fscr.line_length);
    if ((long)fbp == -1) {
        cerr << "error: Error: failed to map framebuffer device to memory." << endl;
        return;
    }

    QImage *img = new QImage((const uchar *)fbp, m_vscr.xres, m_vscr.yres, m_fscr.line_length,
         (m_vscr.bits_per_pixel == 16)?(QImage::QImage::Format_RGB16):(QImage::QImage::Format_RGB32));


    if (!img->save(name, "PNG")) { //writes image into file name in PNG format
        cerr << "error: can't save " << name.toStdString() << " file in PNG format" << endl;
    }
    delete img;

    int ret = munmap(fbp, buffer_size);
    if (ret != 0) {
        cerr << "err: munmap return not zero: " << ret << endl;
    }
    close( m_fb_fd );
}

void Sserver::printFBinfo(const QString &pathFB)
{
    if ( !openFB( pathFB ) ) {
        cerr << "Frame Buffer do not opens" << endl;
        close( m_fb_fd );
        return;
    }

    printf("struct fb_var_screeninfo:\n");
    printf(" xres:           %d\n", m_vscr.xres);
    printf(" yres:           %d\n", m_vscr.yres);
    printf(" xres_virt:      %d\n", m_vscr.xres_virtual);
    printf(" yres_virt:      %d\n", m_vscr.yres_virtual);
    printf(" xoffset:        %d\n", m_vscr.xoffset);
    printf(" yoffset:        %d\n", m_vscr.yoffset);
    printf(" bits_per_pixel: %d\n", m_vscr.bits_per_pixel);
    printf(" grayscale:      %s\n", (m_vscr.grayscale)?"grayscale":"color");
    printf(" red:\n");
    printf("      offset:    %d\n", m_vscr.red.offset);
    printf("      lenght:    %d\n", m_vscr.red.length);
    printf("      msb_right: %s\n", (m_vscr.red.msb_right)?"true":"false");
    printf(" green:\n");
    printf("      offset:    %d\n", m_vscr.green.offset);
    printf("      lenght:    %d\n", m_vscr.green.length);
    printf("      msb_right: %s\n", (m_vscr.green.msb_right)?"true":"false");
    printf(" blue:\n");
    printf("      offset:    %d\n", m_vscr.blue.offset);
    printf("      lenght:    %d\n", m_vscr.blue.length);
    printf("      msb_right: %s\n", (m_vscr.blue.msb_right)?"true":"false");
    printf(" transparent:\n");
    printf("      offset:    %d\n", m_vscr.transp.offset);
    printf("      lenght:    %d\n", m_vscr.transp.length);
    printf("      msb_right: %s\n", (m_vscr.transp.msb_right)?"true":"false");

    printf("\nstruct fb_fix_screeninfo:\n");
    printf(" id:          %s\n", m_fscr.id);
    printf(" smem_len:    %d\n", m_fscr.smem_len);
    printf(" type:        ");
    switch (m_fscr.type){
    case FB_TYPE_PACKED_PIXELS: printf("Packed Pixels\n");
        break;
    case FB_TYPE_PLANES: printf("Non interleaved planes\n");
        break;
    case FB_TYPE_INTERLEAVED_PLANES: printf("Interleaved planes\n");
        break;
    case FB_TYPE_TEXT: printf("Text/attributes\n");
        break;
    case FB_TYPE_VGA_PLANES: printf("EGA/VGA planes\n");
        break;
    case FB_TYPE_FOURCC: printf("Type identified by a V4L2 FOURCC\n");
        break;
    default: printf("Undefinded\n");
    }
    printf(" visual:      ");
    switch (m_fscr.visual){
    case FB_VISUAL_MONO01: printf("Monochr. 1=Black 0=White\n");
        break;
    case FB_VISUAL_MONO10: printf("Monochr. 1=White 0=Black\n");
        break;
    case FB_VISUAL_TRUECOLOR: printf("True color\n");
        break;
    case FB_VISUAL_PSEUDOCOLOR: printf("Pseudo color (like atari)\n");
        break;
    case FB_VISUAL_DIRECTCOLOR: printf("Direct color\n");
        break;
    case FB_VISUAL_STATIC_PSEUDOCOLOR: printf("Pseudo color readonly\n");
        break;
    case FB_VISUAL_FOURCC: printf("Visual identified by a V4L2 FOURCC\n");
        break;
    default: printf("Undefinded\n");
    }
    printf(" line_length: %d\n", m_fscr.line_length);
    printf(" mmio_len:    %d\n", m_fscr.mmio_len);
    close( m_fb_fd );
}

void Sserver::exitQt(int sig)
{
    cout << endl << "Shutdown application CTRL+C (sig = " << sig << ")" << endl;
    QCoreApplication::exit(0);
}
