#include "nvrSysConfigMgr.h"
#include <stdio.h>
#include <stdlib.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/ioctl.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<net/if.h>
#include <unistd.h>

NvrSysConfigMgr *NvrSysConfigMgr::Inst()
{
    static NvrSysConfigMgr* nvrSysConfigMgr = NULL;
    if(!nvrSysConfigMgr){
     nvrSysConfigMgr = new NvrSysConfigMgr();
    }
    return  nvrSysConfigMgr;
}

NvrSysConfigMgr::NvrSysConfigMgr()
{
    cameraInfoList = (NET_DVR_CAMERAINFO *)malloc(CAMERA_MAX_NUM*sizeof(NET_DVR_CAMERAINFO));

    memset(cameraInfoList,0,CAMERA_MAX_NUM*sizeof(NET_DVR_CAMERAINFO));
    pNvrInfo = (NET_DVR_IPDEVINFO *)malloc(sizeof(NET_DVR_IPDEVINFO));
    memset(pNvrInfo,0,sizeof(NET_DVR_IPDEVINFO));

    pNvrInfo->cameraList = cameraInfoList;

}

NvrSysConfigMgr::~NvrSysConfigMgr()
{
    free(cameraInfoList);
    free(pNvrInfo);
}

#include <QCoreApplication>
#include <QtXml> //也可以include <QDomDocument>


const char sysFileName[] = "sysConfig.xml";

//创建初始配置
//void createSysConfig()
//{
//    //打开或创建文件
//    QFile file("sysConfig.xml"); //相对路径、绝对路径、资源路径都可以
//    if(!file.open(QFile::WriteOnly|QFile::Truncate)) //可以用QIODevice，Truncate表示清空原来的内容
//        return;

//    QDomDocument doc;
//    //写入xml头部
//    QDomProcessingInstruction instruction; //添加处理命令
//    instruction=doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\"");
//    doc.appendChild(instruction);
//    //添加根节点
//    QDomElement root=doc.createElement("ZmjNvr");
//    doc.appendChild(root);

//    //增加sub camera 节点
//    QDomElement subCamera=doc.createElement("subCamera");
//    //添加第一个子节点及其子元素
//    QDomElement camera=doc.createElement("camera");
//    camera.setAttribute("id",0); //方式一：创建属性  其中键值对的值可以是各种类型
//    QDomAttr time=doc.createAttribute("time"); //方式二：创建属性 值必须是字符串
//    time.setValue("2013/6/13");
//    camera.setAttributeNode(time);
//    QDomElement cameraIp=doc.createElement("IP"); //创建子元素
//    QDomText text; //设置括号标签中间的值
//    text=doc.createTextNode("192.168.8.11");
//    camera.appendChild(cameraIp);
//    cameraIp.appendChild(text);
//    QDomElement cameraAccount=doc.createElement("account"); //创建子元素
//    text=doc.createTextNode("admin");
//    cameraAccount.appendChild(text);
//    camera.appendChild(cameraAccount);

//    QDomElement passward=doc.createElement("password"); //创建子元素
//    text=doc.createTextNode("zmj123456");
//    passward.appendChild(text);
//    camera.appendChild(passward);


//    subCamera.appendChild(camera);
//    root.appendChild(subCamera);

//    //输出到文件
//    QTextStream out_stream(&file);
//    doc.save(out_stream,4); //缩进4格
//    file.close();

//}



void NvrSysConfigMgr::createSysConfig()
{
    //打开或创建文件
    QFile file(sysFileName); //相对路径、绝对路径、资源路径都可以
    if(!file.open(QFile::WriteOnly|QFile::Truncate)) //可以用QIODevice，Truncate表示清空原来的内容
        return;

    QDomDocument doc;
    //写入xml头部
    QDomProcessingInstruction instruction; //添加处理命令
    instruction=doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(instruction);
    //添加根节点
    QDomElement root=doc.createElement("ZmjNvr");
    doc.appendChild(root);
/************************dev Info***************************************/
    QDomElement devInfoEL=doc.createElement("devInfo");
    QDomElement devIp=doc.createElement("IP"); //创建子元素
    QDomText text; //设置括号标签中间的值
    text=doc.createTextNode("192.168.8.38");
    devIp.appendChild(text);
    devInfoEL.appendChild(devIp);


    QDomElement devChn=doc.createElement("ChnMux"); //创建子元素
    text=doc.createTextNode("4");
    devChn.appendChild(text);
    devInfoEL.appendChild(devChn);


    QDomElement cameraNumEl=doc.createElement("cameraNum"); //创建子元素
    text=doc.createTextNode("4");
    cameraNumEl.appendChild(text);
    devInfoEL.appendChild(cameraNumEl);


    QDomElement labelStates=doc.createElement("isLabelShow"); //创建子元素
    text=doc.createTextNode("1");
    labelStates.appendChild(text);
    devInfoEL.appendChild(labelStates);

    QDomElement zmjTitleEl=doc.createElement("zmjTitle"); //创建子元素
    text=doc.createTextNode("郑州煤矿机械集团股份有限公司");
    zmjTitleEl.appendChild(text);
    devInfoEL.appendChild(zmjTitleEl);

    root.appendChild(devInfoEL);

/************************sub camera***************************************/

    //增加sub camera 节点
    QDomElement subCamera=doc.createElement("subCamera");
    //添加第一个子节点及其子元素
    QDomElement camera=doc.createElement("camera");
    camera.setAttribute("id",0); //方式一：创建属性  其中键值对的值可以是各种类型
    QDomAttr time=doc.createAttribute("name"); //方式二：创建属性 值必须是字符串
    time.setValue("camera0");
    camera.setAttributeNode(time);
    QDomElement cameraIp=doc.createElement("IP"); //创建子元素
    //QDomText text; //设置括号标签中间的值
    text=doc.createTextNode("192.168.8.17");
    camera.appendChild(cameraIp);
    cameraIp.appendChild(text);
    QDomElement cameraAccount=doc.createElement("account"); //创建子元素
    text=doc.createTextNode("admin");
    cameraAccount.appendChild(text);
    camera.appendChild(cameraAccount);

    QDomElement passward=doc.createElement("password"); //创建子元素
    text=doc.createTextNode("zmj123456");
    passward.appendChild(text);
    camera.appendChild(passward);


    QDomElement camereChnId=doc.createElement("ChnId"); //创建子元素
    text=doc.createTextNode("0");
    camereChnId.appendChild(text);
    camera.appendChild(camereChnId);


    QDomElement camereIsrun=doc.createElement("isRun"); //创建子元素
    text=doc.createTextNode("1");
    camereIsrun.appendChild(text);
    camera.appendChild(camereIsrun);




    subCamera.appendChild(camera);
    root.appendChild(subCamera);


    //输出到文件
    QTextStream out_stream(&file);
    doc.save(out_stream,4); //缩进4格
    file.close();
}

void NvrSysConfigMgr::addSubCamara(int id, char *name, char *ip, char *account, char *passward,int chnId,int isRun)
{
    //打开文件
    QFile file(sysFileName); //相对路径、绝对路径、资源路径都可以
    if(!file.open(QFile::ReadOnly))
        return;

    //增加一个一级子节点以及元素
    QDomDocument doc;
    if(!doc.setContent(&file))
    {
        file.close();
        return;
    }
    file.close();

    QDomNodeList list=doc.elementsByTagName("subCamera"); //由标签名定位
    QDomElement subCamera=list.at(0).toElement();
    //添加第一个子节点及其子元素
    QDomElement camera=doc.createElement("camera");
    camera.setAttribute("id",id); //方式一：创建属性  其中键值对的值可以是各种类型
    QDomAttr time=doc.createAttribute("name"); //方式二：创建属性 值必须是字符串
    time.setValue(QString("%1").arg(name));
    camera.setAttributeNode(time);
    QDomElement cameraIp=doc.createElement("IP"); //创建子元素
    QDomText text; //设置括号标签中间的值
    text=doc.createTextNode(QString("%1").arg(ip));
    camera.appendChild(cameraIp);
    cameraIp.appendChild(text);
    QDomElement cameraAccount=doc.createElement("account"); //创建子元素
    text=doc.createTextNode(QString("%1").arg(account));
    cameraAccount.appendChild(text);
    camera.appendChild(cameraAccount);

    QDomElement cameraPasswardEl=doc.createElement("password"); //创建子元素
    text=doc.createTextNode(QString("%1").arg(passward));
    cameraPasswardEl.appendChild(text);
    camera.appendChild(cameraPasswardEl);

    QDomElement camereChnId=doc.createElement("ChnId"); //创建子元素
    text=doc.createTextNode(QString("%1").arg(chnId));
    camereChnId.appendChild(text);
    camera.appendChild(camereChnId);

    QDomElement camereIsrun=doc.createElement("isRun"); //创建子元素
    text=doc.createTextNode(QString("%1").arg(isRun));
    camereIsrun.appendChild(text);
    camera.appendChild(camereIsrun);


    subCamera.appendChild(camera);


    if(!file.open(QFile::WriteOnly|QFile::Truncate)) //先读进来，再重写，如果不用truncate就是在后面追加内容，就无效了
        return;
    //输出到文件
    QTextStream out_stream(&file);
    doc.save(out_stream,4); //缩进4格
    file.close();
}

int NvrSysConfigMgr::getAllSubcamara(NET_DVR_CAMERAINFO *cameraInfo)
{
    //打开或创建文件
    QFile file(sysFileName); //相对路径、绝对路径、资源路径都行
    if(!file.open(QFile::ReadOnly))
        return -1;

    QDomDocument doc;
    int count = 0;
    if(!doc.setContent(&file))
    {
        file.close();
        return -1;
    }
    file.close();


    QDomElement root=doc.documentElement(); //返回根节点
    qDebug()<<root.nodeName();


    QDomNodeList list=doc.elementsByTagName("subCamera"); //由标签名定位
    //QDomNode node=list.at(0);


    QDomNode subCameraNode=list.at(0);
    if(subCameraNode.isElement()){
        QDomNode node=subCameraNode.firstChild(); //获得第一个子节点
        while(!node.isNull())  //如果节点不空
        {
            if(node.isElement()) //如果节点是元素
            {

                QDomElement e=node.toElement(); //转换为元素，注意元素和节点是两个数据结构，其实差不多
                qDebug()<<"pp:"<<e.tagName()<<" "<<e.attribute("id")<<" "<<e.attribute("name"); //打印键值对，tagName和nodeName是一个东西
                cameraInfo[count].cameraID = e.attribute("id").toInt();
                memcpy(cameraInfo[count].sCameraName,e.attribute("name").toStdString().c_str(),e.attribute("name").size());
                QDomNodeList list=e.childNodes();
                for(int i=0;i<list.count();i++) //遍历子元素，count和size都可以用,可用于标签数计数
                {
                    QDomNode n=list.at(i);
                    QString nodeStr = n.nodeName();
                    QString nodeValStr = n.toElement().text();
                    // printf("------%s::%d\n",nodeValStr.toStdString().c_str(),nodeValStr.size());
                    if(0==nodeStr.compare(QString("IP"))){
                        memcpy(cameraInfo[count].struIP.sIpV4,nodeValStr.toStdString().c_str(),nodeValStr.size());
                       // printf("*********ip*******%s\n",cameraInfo[count].struIP.sIpV4);
                    }else if(0==nodeStr.compare(QString("account"))){
                        memcpy(cameraInfo[count].sUserName,nodeValStr.toStdString().c_str(),nodeValStr.size());
                       // printf("******use*****%s\n",cameraInfo[count].sUserName);
                    }else if(0==nodeStr.compare(QString("password"))){
                        memcpy(cameraInfo[count].sPassword,nodeValStr.toStdString().c_str(),nodeValStr.size());
                       // printf("*****pw******%s\n",cameraInfo[count].sPassword);
                    }else if(0==nodeStr.compare(QString("ChnId"))){
                        cameraInfo[count].cmaraChnID = nodeValStr.toInt();
                        // printf("******chnID*****%d\n",cameraInfo[count].cmaraChnID);
                    }else if(0==nodeStr.compare(QString("isRun"))){
                        cameraInfo[count].isRun = nodeValStr.toInt();
                         //printf("******isRun*****%d\n",cameraInfo[count].isRun);
                    }
                      // qDebug()<<n.nodeName()<<":"<<n.toElement().text()<<nodeValStr.size();
                }
//                printf("%d:::%d::name:%s:iP:%s:user:%s:pw:%s:%d\n"
//                       ,count \
//                       ,cameraInfo[count].cameraID \
//                        ,cameraInfo[count].sCameraName \
//                       ,cameraInfo[count].struIP.sIpV4 \
//                      ,cameraInfo[count].sUserName \
//                     ,cameraInfo[count].sPassword \
//                    ,cameraInfo[count].cmaraChnID);
                count++;
            }
            node=node.nextSibling(); //下一个兄弟节点,nextSiblingElement()是下一个兄弟元素，都差不多
        }
    }
    return 0;

}

int NvrSysConfigMgr::getNvrLocalInfo(NET_DVR_IPDEVINFO *nvrInfo)
{
    //打开或创建文件
    QFile file(sysFileName); //相对路径、绝对路径、资源路径都行
    if(!file.open(QFile::ReadOnly))
        return -1;

    QDomDocument doc;
    int count = 0;
    if(!doc.setContent(&file))
    {
        file.close();
        return -1;
    }
    file.close();


    QDomElement root=doc.documentElement(); //返回根节点
    qDebug()<<root.nodeName();


    QDomNodeList list=doc.elementsByTagName("devInfo"); //由标签名定位
    //QDomNode node=list.at(0);


   // QDomNode subCameraNode=list.at(0);
    QDomNode node=list.at(0); //获得第一个子节点
    while(!node.isNull())  //如果节点不空
    {
        if(node.isElement()) //如果节点是元素
        {

            QDomElement e=node.toElement(); //转换为元素，注意元素和节点是两个数据结构，其实差不多
            QDomNodeList list=e.childNodes();
            for(int i=0;i<list.count();i++) //遍历子元素，count和size都可以用,可用于标签数计数
            {
                QDomNode n=list.at(i);
                QString nodeStr = n.nodeName();
                QString nodeValStr = n.toElement().text();
                 printf("------%s::%d\n",nodeValStr.toStdString().c_str(),nodeValStr.size());
                 int size = nodeValStr.toLocal8Bit().length();
                if(0==nodeStr.compare(QString("IP"))){
                    memcpy(nvrInfo->struIP.sIpV4,nodeValStr.toStdString().c_str(),size);
                   // printf("*********ip*******%s\n",cameraInfo[count].struIP.sIpV4);
                }else if(0==nodeStr.compare(QString("ChnMux"))){
                    nvrInfo->cmaraChnNum = nodeValStr.toInt();
                    // printf("******chnID*****%d\n",cameraInfo[count].cmaraChnID);
                }else if(0==nodeStr.compare(QString("cameraNum"))){
                    nvrInfo->cameraNum = nodeValStr.toInt();
                    // printf("******chnID*****%d\n",cameraInfo[count].cmaraChnID);
                }else if(0==nodeStr.compare(QString("isLabelShow"))){
                    nvrInfo->isLabelShow = nodeValStr.toInt();
                    // printf("******chnID*****%d\n",cameraInfo[count].cmaraChnID);
                }else if(0==nodeStr.compare(QString("zmjTitle"))){
                     memcpy(nvrInfo->sTitleName,nodeValStr.toStdString().c_str(),size);
                     printf("******sTitleName*****%s   %d\n",nvrInfo->sTitleName,size);
                }
                    qDebug()<<n.nodeName()<<":"<<n.toElement().text()<<size;
            }
            printf("---------nvr info-----%d:iP:%s:%d\n"
                   ,count \
                  ,nvrInfo->struIP.sIpV4
                ,nvrInfo->cmaraChnNum);
            count++;
        }
        node=node.nextSibling(); //下一个兄弟节点,nextSiblingElement()是下一个兄弟元素，都差不多
    }

    getAllSubcamara(nvrInfo->cameraList);
//    if(subCameraNode.isElement()){

//    }
    return 0;

}

int NvrSysConfigMgr::setNvrLocalInfo(NET_DVR_IPDEVINFO *nvrInfo)
{
    int i=0;
    //打开或创建文件
    QFile file(sysFileName); //相对路径、绝对路径、资源路径都可以
    if(!file.open(QFile::WriteOnly|QFile::Truncate)) //可以用QIODevice，Truncate表示清空原来的内容
        return -1;

    QDomDocument doc;
    //写入xml头部
    QDomProcessingInstruction instruction; //添加处理命令
    instruction=doc.createProcessingInstruction("xml","version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(instruction);
    //添加根节点
    QDomElement root=doc.createElement("ZmjNvr");
    doc.appendChild(root);
/************************dev Info***************************************/
    QDomElement devInfoEL=doc.createElement("devInfo");
    QDomElement devIp=doc.createElement("IP"); //创建子元素
    QDomText text; //设置括号标签中间的值
    text=doc.createTextNode(QString("%1").arg(nvrInfo->struIP.sIpV4));
    devIp.appendChild(text);
    devInfoEL.appendChild(devIp);


    QDomElement devChn=doc.createElement("ChnMux"); //创建子元素
    text=doc.createTextNode(QString("%1").arg(nvrInfo->cmaraChnNum));
    devChn.appendChild(text);
    devInfoEL.appendChild(devChn);


    QDomElement cameraNumEl=doc.createElement("cameraNum"); //创建子元素
    text=doc.createTextNode(QString("%1").arg(nvrInfo->cameraNum));
    cameraNumEl.appendChild(text);
    devInfoEL.appendChild(cameraNumEl);


    QDomElement labelStates=doc.createElement("isLabelShow"); //创建子元素
    text=doc.createTextNode(QString("%1").arg(nvrInfo->isLabelShow));
    labelStates.appendChild(text);
    devInfoEL.appendChild(labelStates);


    QDomElement zmjTitleEl=doc.createElement("zmjTitle"); //创建子元素
    text=doc.createTextNode(QString("%1").arg((char *)nvrInfo->sTitleName));
    zmjTitleEl.appendChild(text);
    devInfoEL.appendChild(zmjTitleEl);

    root.appendChild(devInfoEL);

/************************sub camera***************************************/
    QDomElement subCamera=doc.createElement("subCamera");
    for(i=0;i<nvrInfo->cameraNum;i++){
        //添加第一个子节点及其子元素
        QDomElement camera=doc.createElement("camera");
        camera.setAttribute("id",nvrInfo->cameraList[i].cameraID); //方式一：创建属性  其中键值对的值可以是各种类型
        QDomAttr time=doc.createAttribute("name"); //方式二：创建属性 值必须是字符串
        time.setValue(QString("%1").arg((char *)nvrInfo->cameraList[i].sCameraName));
        camera.setAttributeNode(time);
        QDomElement cameraIp=doc.createElement("IP"); //创建子元素
        QDomText text; //设置括号标签中间的值
        text=doc.createTextNode(QString("%1").arg(nvrInfo->cameraList[i].struIP.sIpV4));
        camera.appendChild(cameraIp);
        cameraIp.appendChild(text);
        QDomElement cameraAccount=doc.createElement("account"); //创建子元素
        text=doc.createTextNode(QString("%1").arg((char*)nvrInfo->cameraList[i].sUserName));
        cameraAccount.appendChild(text);
        camera.appendChild(cameraAccount);

        QDomElement cameraPasswardEl=doc.createElement("password"); //创建子元素
        text=doc.createTextNode(QString("%1").arg((char *)nvrInfo->cameraList[i].sPassword));
        cameraPasswardEl.appendChild(text);
        camera.appendChild(cameraPasswardEl);

        QDomElement camereChnId=doc.createElement("ChnId"); //创建子元素
        text=doc.createTextNode(QString("%1").arg(nvrInfo->cameraList[i].cmaraChnID));
        camereChnId.appendChild(text);
        camera.appendChild(camereChnId);

        QDomElement camereIsRun=doc.createElement("isRun"); //创建子元素
        text=doc.createTextNode(QString("%1").arg(nvrInfo->cameraList[i].isRun));
        camereIsRun.appendChild(text);
        camera.appendChild(camereIsRun);



        subCamera.appendChild(camera);
    }
    root.appendChild(subCamera);


    //输出到文件
    QTextStream out_stream(&file);
    doc.save(out_stream,4); //缩进4格
    file.close();
    return 0;
}

int NvrSysConfigMgr::setNvrLocalInfo()
{
    setNvrLocalInfo(pNvrInfo);
}

int NvrSysConfigMgr::getNvrTitle(char *name, int *len)
{
    //打开或创建文件
    QFile file(sysFileName); //相对路径、绝对路径、资源路径都行
    if(!file.open(QFile::ReadOnly))
        return -1;

    QDomDocument doc;
    int count = 0;
    if(!doc.setContent(&file))
    {
        file.close();
        return -1;
    }
    file.close();


    QDomElement root=doc.documentElement(); //返回根节点
    qDebug()<<root.nodeName();


    QDomNodeList list=doc.elementsByTagName("devInfo"); //由标签名定位
    //QDomNode node=list.at(0);


   // QDomNode subCameraNode=list.at(0);
    QDomNode node=list.at(0); //获得第一个子节点
    while(!node.isNull())  //如果节点不空
    {
        if(node.isElement()) //如果节点是元素
        {

            QDomElement e=node.toElement(); //转换为元素，注意元素和节点是两个数据结构，其实差不多
            QDomNodeList list=e.childNodes();
            for(int i=0;i<list.count();i++) //遍历子元素，count和size都可以用,可用于标签数计数
            {
                QDomNode n=list.at(i);
                QString nodeStr = n.nodeName();
                QString nodeValStr = n.toElement().text();
                 printf("------%s::%d\n",nodeValStr.toStdString().c_str(),nodeValStr.size());
                 int size = nodeValStr.toLocal8Bit().length();
                if(0==nodeStr.compare(QString("zmjTitle"))){
                     memcpy(name,nodeValStr.toStdString().c_str(),size);
                     *len = size;
                     printf("******get TitleName*****%s   %d\n",name,size);
                }
                    qDebug()<<n.nodeName()<<":"<<n.toElement().text()<<size;
            }
            count++;
        }
        node=node.nextSibling(); //下一个兄弟节点,nextSiblingElement()是下一个兄弟元素，都差不多
    }


//    if(subCameraNode.isElement()){

//    }
    return 0;

}

int NvrSysConfigMgr::setNvrTitle(char *name, int size)
{

}

NET_DVR_IPDEVINFO *NvrSysConfigMgr::getNvrInfo()
{
    qDebug()<<"123";
    getNvrLocalInfo(pNvrInfo);
     qDebug()<<"456";
     return pNvrInfo;
}

NET_DVR_IPDEVINFO *NvrSysConfigMgr::getCacheNvrInfo()
{
    return pNvrInfo;
}

int NvrSysConfigMgr::setip(char *ip)
{
    struct ifreq temp;
       struct sockaddr_in *addr;
       int fd = 0;
       int ret = -1;
       strcpy(temp.ifr_name, "eth0");
       if((fd=socket(AF_INET, SOCK_STREAM, 0))<0)
       {
         return -1;
       }
       addr = (struct sockaddr_in *)&(temp.ifr_addr);
       addr->sin_family = AF_INET;
       addr->sin_addr.s_addr = inet_addr(ip);
       ret = ioctl(fd, SIOCSIFADDR, &temp);
       close(fd);
       if(ret < 0)
          return -1;
       return 0;
}

char *NvrSysConfigMgr::getip(char *ip_buf)
{
    struct ifreq temp;
       struct sockaddr_in *myaddr;
       int fd = 0;
       int ret = -1;
       strcpy(temp.ifr_name, "eth0");
       if((fd=socket(AF_INET, SOCK_STREAM, 0))<0)
       {
           return NULL;
       }
       ret = ioctl(fd, SIOCGIFADDR, &temp);
       close(fd);
       if(ret < 0)
           return NULL;
       myaddr = (struct sockaddr_in *)&(temp.ifr_addr);
       strcpy(ip_buf, inet_ntoa(myaddr->sin_addr));
       return ip_buf;
}

int NvrSysConfigMgr::resetSys()
{
    createSysConfig();//17
    addSubCamara(1,"camera1","192.168.8.11","admin","zmj123456",1,1);
    addSubCamara(2,"camera2","192.168.8.22","admin","zmj123456",2,1);
    addSubCamara(3,"camera3","192.168.8.24","admin","zmj123456",3,0);
}
