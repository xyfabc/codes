#ifndef NVRSERVERPARATYPE_H
#define NVRSERVERPARATYPE_H



///* IP设备结构 */
//typedef struct
//{
//    DWORD dwEnable;                    /* 该IP设备是否启用 */
//    BYTE sUserName[NAME_LEN];        /* 用户名 */
//    BYTE sPassword[PASSWD_LEN];        /* 密码 */
//    NET_DVR_IPADDR struIP;            /* IP地址 */
//    WORD wDVRPort;                     /* 端口号 */
//    BYTE byRes[34];                /* 保留 */
//}NET_DVR_IPDEVINFO, *LPNET_DVR_IPDEVINFO;

//#define  DEV_ID_LEN           32    //设备ID长度

////ipc接入设备信息扩展，支持ip设备的域名添加
//typedef struct tagNET_DVR_IPDEVINFO_V31
//{
//    BYTE byEnable;                    //该IP设备是否有效
//    BYTE byProType;                    //协议类型，0-私有协议，1-松下协议，2-索尼
//    BYTE byEnableQuickAdd;        // 0 不支持快速添加  1 使用快速添加
//    // 快速添加需要设备IP和协议类型，其他信息由设备默认指定
//    BYTE byRes1;                    //保留字段，置0
//    BYTE sUserName[NAME_LEN];        //用户名
//    BYTE sPassword[PASSWD_LEN];        //密码
//    BYTE byDomain[MAX_DOMAIN_NAME];    //设备域名
//    NET_DVR_IPADDR struIP;            //IP地址
//    WORD wDVRPort;                     // 端口号
//    BYTE  szDeviceID[DEV_ID_LEN];  //设备ID
//    BYTE byRes2[2];                //保留字段，置0
//}NET_DVR_IPDEVINFO_V31, *LPNET_DVR_IPDEVINFO_V31;


//// 流信息 - 72字节长
//typedef struct tagNET_DVR_STREAM_INFO
//{
//    DWORD dwSize;
//    BYTE  byID[STREAM_ID_LEN];      //ID数据
//    DWORD dwChannel;                //关联设备通道，等于0xffffffff时，表示不关联
//    BYTE  byRes[32];                //保留
//}NET_DVR_STREAM_INFO, *LPNET_DVR_STREAM_INFO;

//typedef struct tagNET_DVR_RTSP_PROTOCAL_CFG
//{
//    BYTE    byEnable;
//    BYTE    byLocalBackUp; //是否本地备份
//    BYTE    byRes[2];
//    BYTE    strURL[URL_LEN_V40];
//    DWORD   dwProtocalType;   //协议类型
//    BYTE    sUserName[NAME_LEN];   //设备登陆用户名
//    BYTE    sPassWord[PASSWD_LEN]; // 设备登陆密码
//    BYTE    byAddress[MAX_DOMAIN_NAME];  //前端IP或者域名,需要设备解析
//    //解析方式为有字母存在且有'.'则认为是域名,否则为IP地址
//    WORD    wPort;
//    BYTE    byRes1[122];             //保留
//}NET_DVR_RTSP_PROTOCAL_CFG, *LPNET_DVR_RTSP_PROTOCAL_CFG;

//typedef union tagNET_DVR_GET_STREAM_UNION
//{
//    NET_DVR_IPCHANINFO      struChanInfo;    /*IP通道信息*/
//    NET_DVR_IPSERVER_STREAM struIPServerStream;  // IPServer去流
//    NET_DVR_PU_STREAM_CFG   struPUStream;     //  通过前端设备获取流媒体去流
//    NET_DVR_DDNS_STREAM_CFG struDDNSStream;     //通过IPServer和流媒体取流
//    NET_DVR_PU_STREAM_URL   struStreamUrl;        //通过流媒体到url取流
//    NET_DVR_HKDDNS_STREAM    struHkDDNSStream;   //通过hiDDNS去取流
//    NET_DVR_IPCHANINFO_V40 struIPChan; //直接从设备取流（扩展）
//}NET_DVR_GET_STREAM_UNION, *LPNET_DVR_GET_STREAM_UNION;

//typedef enum
//{
//    NET_SDK_IP_DEVICE = 0,
//        NET_SDK_STREAM_MEDIA,
//        NET_SDK_IPSERVER,
//        NET_SDK_DDNS_STREAM_CFG,
//        NET_SDK_STREAM_MEDIA_URL,
//        NET_SDK_HKDDNS,
//        NET_SDK_IP_DEVICE_ADV,
//        NET_SDK_IP_DEVICE_V40,
//        NET_SDK_RTSP
//}GET_STREAM_TYPE;

//typedef struct tagNET_DVR_STREAM_MODE
//{
//    BYTE    byGetStreamType; //取流方式GET_STREAM_TYPE，0-直接从设备取流，1-从流媒体取流、2-通过IPServer获得ip地址后取流,3.通过IPServer找到设备，再通过流媒体去设备的流
//    //4-通过流媒体由URL去取流,5-通过hkDDNS取流，6-直接从设备取流(扩展)，使用NET_DVR_IPCHANINFO_V40结构, 7-通过RTSP协议方式进行取流
//    BYTE    byRes[3];        //保留字节
//    NET_DVR_GET_STREAM_UNION uGetStream;    // 不同取流方式结构体
//}NET_DVR_STREAM_MODE, *LPNET_DVR_STREAM_MODE;


////扩展IP接入配置设备
//typedef struct tagNET_DVR_IPPARACFG_V40
//{
//    DWORD      dwSize;                            /* 结构大小 */
//    DWORD       dwGroupNum;                    //     设备支持的总组数
//    DWORD      dwAChanNum;                    //最大模拟通道个数
//    DWORD      dwDChanNum;                  //数字通道个数
//    DWORD      dwStartDChan;                    //起始数字通道
//    BYTE       byAnalogChanEnable[MAX_CHANNUM_V30];    /* 模拟通道是否启用，从低到高表示1-64通道，0表示无效 1有效 */
//    NET_DVR_IPDEVINFO_V31   struIPDevInfo[MAX_IP_DEVICE_V40];      /* IP设备 */
//    NET_DVR_STREAM_MODE  struStreamMode[MAX_CHANNUM_V30];
//    BYTE            byRes2[20];                 // 保留字节
//}NET_DVR_IPPARACFG_V40, *LPNET_DVR_IPPARACFG_V40;


#endif // NVRSERVERPARATYPE_H
