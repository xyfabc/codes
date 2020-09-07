#ifndef CAMERA_H
#define CAMERA_H



typedef unsigned char BYTE;
typedef unsigned short WORD;
#define NAME_LEN                32      //用户名长度
#define IP_ADDR_LEN              64      //用户名长度
#define TITEL_NAME_LEN          128      //用户名长度
#define PASSWD_LEN              16      //密码长度
#define PASSWD_LEN              16      //密码长度
#define CAMERA_MAX_NUM          128


#define NVR_SOFT_VERSION "v1.0.0_20200305"


/*
IP地址
*/
typedef struct
{
    char    sIpV4[64];                        /* IPv4地址 */
    //BYTE    byIPv6[128];                        /* 保留 */
}NET_DVR_IPADDR, *LPNET_DVR_IPADDR;


typedef struct tagNET_DVR_CAMERAINFO
{
    int cameraID;                   //摄像头ID号
    int cmaraChnID;                 //对应显示通道ID号
    int isRun;                          //是否运行
    BYTE sCameraName[NAME_LEN];        //摄像头名称
    BYTE sUserName[NAME_LEN];        //用户名
    BYTE sPassword[PASSWD_LEN];        //密码
    NET_DVR_IPADDR struIP;            //IP地址

}NET_DVR_CAMERAINFO, *LPNET_DVR_CAMERAINFO;

typedef struct tagNET_DVR_IPDEVINFO
{
    int cmaraChnNum;                 //显示通道数量
    int cameraNum;                   //摄像头数量
    int isLabelShow;                 //是否显示通道信息
    BYTE sUserName[NAME_LEN];        //用户名
    BYTE sPassword[PASSWD_LEN];        //密码
    NET_DVR_IPADDR struIP;            //IP地址
    BYTE sTitleName[TITEL_NAME_LEN];        //zmj 抬头名称
    NET_DVR_CAMERAINFO *cameraList;     //摄像头列表

}NET_DVR_IPDEVINFO, *LPNET_DVR_IPDEVINFO;


class Camera
{
public:
    Camera();

private:

};

#endif // CAMERA_H
