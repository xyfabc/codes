

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <pthread.h>

#include "camera.h"
#include "watchdog.h"
#include "framebuffer.h"
#include "hifb.h"
#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vb.h"


#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vpss.h"
#include "mpi_vdec.h"
#include "mpi_hdmi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_region.h"
#include "sample_comm.h"

#include "zmj_nvr_cfg.h"
#include "nvr_watchdog.h"


#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <string.h>
#include "framebuffer.h"


//以下代码来自libminigui-1.6.10/src/ial/native/native.c
//需要在文本控制台下运行,才能获取到数据,在ubuntu 8.10 GNOME界面只能收到1个字节数据0xfa
//InitIAL==>__mg_cur_input = inputs{"console", InitNativeInput, TermNativeInput},
//mousedev = mousedev_IMPS2;
//input->update_mouse = mouse_update;

/* Mouse button bits */
#define WHEEL_UP 0x10
#define WHEEL_DOWN 0x08
#define BUTTON_L 0x04
#define BUTTON_M 0x02
#define BUTTON_R 0x01
#define SCALE 3 /* default scaling factor for acceleration */
#define THRESH 5 /* default threshhold for acceleration */

static int xpos; /* current x position of mouse */
static int ypos; /* current y position of mouse */
static int minx; /* minimum allowed x position */
static int maxx; /* maximum allowed x position */
static int miny; /* minimum allowed y position */
static int maxy; /* maximum allowed y position */
static int buttons; /* current state of buttons */
static int scale = SCALE; /* acceleration scale factor */
static int thresh = THRESH;/* acceleration threshhold */
static int mouse_update(int dx, int dy, int dz);
static int IMPS2_Read (int *dx, int *dy, int *dz, int *bp);
static void mouse_setposition (int newx, int newy);
static void mouse_setrange (int newminx, int newminy, int newmaxx, int newmaxy);
int mouse_fd;


void u32_u8_be(int nu, BYTE *buf)
{
    buf[0] = (BYTE)(nu >> 24);
    buf[1] = (BYTE)(nu >> 16);
    buf[2] = (BYTE)(nu >> 8);
    buf[3] = (BYTE)(nu >> 0);
}

#define LCD_INIT_TEST

char write_fifo_name[]="/home/zmj/mouse_pos_fifo";

int main(void)
{
    hifb_init();
   // SAMPLE_HIFB_StandardMode();

#ifdef LCD_INIT_TEST
    int write_fd;
    mkfifo(write_fifo_name, S_IRUSR|S_IWUSR);
    printf("SYS init !\n");
    write_fd = open(write_fifo_name,O_WRONLY);
    if(write_fd<0)
    {
        perror("open_w");
        exit(-1);
    }
#else
    while(1){
       sleep(1);
    }
#endif
    int dx,dy,dz;
    static unsigned char imps2_param [] = {243,200,243,100,243,80};//,242};

    // 来自vnc4的xc/programs/Xserver/hw/xfree86/input/mouse/mouse.c==> PROT_IMPS2
    const char *mdev="/dev/input/mice";
    mouse_fd = open (mdev, O_RDWR); // | O_NONBLOCK);

    if (mouse_fd < 0) {
        perror("open mouse error");
        mouse_fd = open (mdev, O_RDONLY); // | O_NONBLOCK);
        if (mouse_fd < 0)
            return -1;
    } else {
        write (mouse_fd, imps2_param, sizeof (imps2_param)); // 初始化序列, 这样可以读取4个字节数据
        // 0x80用来表示滚轮向上还是向下滚动
        // 0xa0表示滚轮向上滚动的同时中键按下
        printf("imps2_param ok!\n");
    }

    mouse_setrange(0, 0, 1920, 1080);
    for (;;) {
        IMPS2_Read(&dx, &dy, &dz, &buttons);
        mouse_update(dx, dy, dz);
        mouse_setposition(xpos, ypos);
        SetMousePos(xpos,ypos);

        BYTE bufw[64] = "";
        bufw[0] = '<';
        bufw[9] = '>';
        u32_u8_be(xpos, &bufw[1]);
        u32_u8_be(ypos, &bufw[5]);
#ifdef LCD_INIT_TEST
        write(write_fd, bufw, 10);
#endif
        //printf("[%04d,%04d,0x%04x]\n", xpos, ypos, buttons);
    }
    return 0;
}

static int IMPS2_Read (int *dx, int *dy, int *dz, int *bp)
{
    static unsigned char buf[5];
    static int buttons[7] = { 0, 1, 3, 0, 2, 0, 0}; // 1左键,2中键,3右键
    static int nbytes;
    int n;

    while ((n = read (mouse_fd, &buf [nbytes], 4 - nbytes))) {
        if (n < 0) {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }

        nbytes += n;
        if (nbytes == 4) {
            int wheel;
            // printf("[luther.gliethttp]: %02x %02x %02x %02x\n", buf[0], buf[1], buf[2], buf[3]);
            if ((buf[0] & 0xc0) != 0) {
                buf[0] = buf[1];
                buf[1] = buf[2];
                buf[2] = buf[3];
                nbytes = 3;
                return -1;
            }
            /* FORM XFree86 4.0.1 */
            *bp = buttons[(buf[0] & 0x07)];
            *dx = (buf[0] & 0x10) ? buf[1] - 256 : buf[1];
            *dy = (buf[0] & 0x20) ? -(buf[2] - 256) : -buf[2];
            /* Is a wheel event? */
            if ((wheel = buf[3]) != 0) {
                if(wheel > 0x7f) {
                    *bp |= WHEEL_UP;
                }
                else {
                    *bp |= WHEEL_DOWN;
                }
            }
            *dz = 0;
            nbytes = 0;
            return 1;
        }
    }
    return 0;
}

static int mouse_update(int dx, int dy, int dz)
{
    int r;
    int sign;
    sign = 1;
    if (dx < 0) {
        sign = -1;
        dx = -dx;
    }
    if (dx > thresh)
        dx = thresh + (dx - thresh) * scale;
    dx *= sign;
    xpos += dx;
    if( xpos < minx )
        xpos = minx;
    if( xpos > maxx )
        xpos = maxx;
    sign = 1;
    if (dy < 0) {
        sign = -1;
        dy = -dy;
    }
    if (dy > thresh)
        dy = thresh + (dy - thresh) * scale;
    dy *= sign;
    ypos += dy;
    if ( ypos < miny )
        ypos = miny;
    if ( ypos > maxy )
        ypos = maxy;
    return 1;
}

static void mouse_setposition (int newx, int newy)
{
    if (newx < minx)
        newx = minx;
    if (newx > maxx)
        newx = maxx;
    if (newy < miny)
        newy = miny;
    if (newy > maxy)
        newy = maxy;
    if (newx == xpos && newy == ypos)
        return;
    xpos = newx;
    ypos = newy;
}

static void mouse_setrange (int newminx, int newminy, int newmaxx, int newmaxy)
{
    minx = newminx;
    miny = newminy;
    maxx = newmaxx;
    maxy = newmaxy;
    mouse_setposition ((newminx + newmaxx) / 2, (newminy + newmaxy) / 2);
}

static int mouse_getbutton (void)
{
    return buttons;
}

static void mouse_getxy (int* x, int* y)
{
    *x = xpos;
    *y = ypos;
}





int main111(int argc, char *argv[])
{

//    //get local configs
//    NET_DVR_IPDEVINFO *pNvrInfo;

//    //启动看门狗

//    NvrSysConfigMgr *pNvrSysConfigMgr = NvrSysConfigMgr::Inst();

//    // pNvrSysConfigMgr->resetSys();

//    //get local config
//    pNvrInfo = pNvrSysConfigMgr->getNvrInfo();




    //hisi framebuffer init
    hifb_init();
    //SAMPLE_HIFB_StandardMode();
    sleep(10);
    while(1){
        sleep(1);
    }

#if 0
    //hisi video init
   // vdec_init(ZMJ_VDEC_MAX_CHN,(SAMPLE_VO_MODE_E)pNvrInfo->cmaraChnNum);

   // RTSPClientPlay(pNvrInfo);

    // TCP server
    //tcpServerInit();

    // next is Qt UI
    printf("---------------------1111-------------mainWindow show-\n");
    QApplication a(argc, argv);
    int id = QFontDatabase::addApplicationFont("/mnt/nfs/qt-hisi/lib/fonts/DroidSansFallback.ttf");
    QString msyh = QFontDatabase::applicationFontFamilies (id).at(0);
    QFont font(msyh,10);
    qDebug()<<msyh;
    font.setPointSize(20);
    a.setFont(font);

    //    QSurfaceFormat fmt;
    //    fmt.setDepthBufferSize(24);

    //    if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL) {
    //        qDebug("Requesting 3.3 compatibility context");
    //        fmt.setVersion(3, 3);
    //        fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    //    } else {
    //        qDebug() << fmt.majorVersion();
    //        qDebug() << fmt.minorVersion();
    //        qDebug("Requesting 3.0 context");
    //        fmt.setVersion(2, 0);
    //    }



    MainWindow w;
    std::string str = (char*)pNvrInfo->sTitleName;
    QString titleName = QString::fromStdString(str);
    w.setTitleName(titleName);
    setMainwindow(&w);
    w.show();
    w.resize(1920, 1080);
    w.setChnLabelText();

  //  SAMPLE_HIFB_StandardMode();
    return a.exec();
#endif




}



