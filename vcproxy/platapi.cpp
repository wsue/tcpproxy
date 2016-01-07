
#include "stdafx.h"
#include <winsock2.h>
#include <Iphlpapi.h>





//      打开一个tcp监听端口
int os_tcp_listen(const char *ipaddr,int port)
{
    struct  sockaddr_in     addr;
    int                     sd;

    sd  = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if( sd == -1 )
    {
        printf("ERROR:  create svr socket fail\n");
        return -1;
    }

    memset(&addr,0,sizeof(addr));

    addr.sin_family     = AF_INET;
    addr.sin_port       = htons(port);

    if( ipaddr )
        addr.sin_addr.s_addr    = inet_addr(ipaddr);

    if ( bind (sd,(struct sockaddr *)&addr,sizeof(sockaddr_in) ) != 0 )
    {
        printf("ERROR:  bind socket fail\n");
        closesocket(sd);
        return -1;
    }

    if( listen (sd,100) != 0 )
    {
        printf("ERROR:  listen socket fail\n");
        closesocket(sd);
        return -1;
    }

    return sd;
}

//	连接到指定IP地址
int os_tcp_connect(const char *ipaddr,int port)
{
    struct  sockaddr_in     addr;
    int                     sd;

    sd  = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if( sd == -1 )
    {
        printf("ERROR:  create svr socket fail\n");
        return -1;
    }

    memset(&addr,0,sizeof(addr));

    addr.sin_family     = AF_INET;
    addr.sin_port       = htons(port);

    if( ipaddr )
        addr.sin_addr.s_addr    = inet_addr(ipaddr);

    if ( connect (sd,(struct sockaddr *)&addr,sizeof(sockaddr_in) ) != 0 )
    {
        printf("ERROR:  bind socket fail\n");
        closesocket(sd);
        return -1;
    }

    return sd;
}

int os_tcp_connect_to(const char *ipaddr,int port,int conn_to)
{
    struct sockaddr_in server;
    int sd  = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if( sd == -1 ){
        printf("ERROR:  create svr socket fail\n");
        return -1;
    }

    int TimeOut = 6000;
    if(::setsockopt(sd,SOL_SOCKET,SO_SNDTIMEO,(char *)&TimeOut,sizeof(TimeOut))==SOCKET_ERROR){
        return 0;
    }
    TimeOut=6000;//设置接收超时6秒
    if(::setsockopt(sd,SOL_SOCKET,SO_RCVTIMEO,(char *)&TimeOut,sizeof(TimeOut))==SOCKET_ERROR){
        return 0;
    }

    //设置非阻塞方式连接
    unsigned long ul = 1;
    int ret = ioctlsocket(sd, FIONBIO, (unsigned long*)&ul);
    if(ret==SOCKET_ERROR)return 0;

    //连接
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr .s_addr = inet_addr((LPCSTR)ipaddr);
    if(server.sin_addr.s_addr == INADDR_NONE){return 0;}

    connect(sd,(const struct sockaddr *)&server,sizeof(server));

    //select 模型，即设置超时
    struct timeval timeout ;
    fd_set r;

    FD_ZERO(&r);
    FD_SET(sd, &r);
    timeout.tv_sec = conn_to; //连接超时15秒
    timeout.tv_usec =0;
    ret = select(0, 0, &r, 0, &timeout);
    if ( ret <= 0 )
    {
        ::closesocket(sd);
        return 0;
    }
    //一般非锁定模式套接比较难控制，可以根据实际情况考虑 再设回阻塞模式
    unsigned long ul1= 0 ;
    ret = ioctlsocket(sd, FIONBIO, (unsigned long*)&ul1);
    if(ret==SOCKET_ERROR){
        ::closesocket (sd);
        return 0;
    }

    return sd;
}

int os_tcp_close(int sd)
{
    return closesocket(sd);
}

int os_writen(int sd,char *buf,int size)
{
    while( size > 0 )
    {
        int ret = send(sd,buf,size,0);
        if( ret <= 0 )
        {
            return -1;
        }

        buf     += ret;
        size    -= ret;
    }

    return 0;
}


int os_startthr( LPTHREAD_START_ROUTINE func,void *param)
{
    /* 创建处理线程 */
    HANDLE  handle = ::CreateThread(NULL, 0, func , param, 0, NULL);
    if( handle == NULL ) {
        return -1;
    }
    else {
        /* 释放线程资源，相当于linux的detach */
        CloseHandle(handle);
    }

    return 0;
}

/* 返回本机IP地址 */
int os_pcapsock_getselfip(char *strIP)
{
    //记录网卡数量
    int         netCardNum          = 0;
    //记录每张网卡上的IP地址数量
    int         IPnumPerNetCard     = 0;
    int         find		    = 0;


    //PIP_ADAPTER_INFO结构体指针存储本机网卡信息
    //得到结构体大小,用于GetAdaptersInfo参数
    //调用GetAdaptersInfo函数,填充pIpAdapterInfo指针变量;其中stSize参数既是一个输入量也是一个输出量
    IP_ADAPTER_INFO                 tTmpAdapter;
    PIP_ADAPTER_INFO                pIpAdapterInfo = &tTmpAdapter;
    unsigned long       stSize          = sizeof(IP_ADAPTER_INFO);

    int                 nRel            = GetAdaptersInfo(pIpAdapterInfo,&stSize);


    if (ERROR_BUFFER_OVERFLOW == nRel)
    {
        //如果函数返回的是ERROR_BUFFER_OVERFLOW
        //则说明GetAdaptersInfo参数传递的内存空间不够,同时其传出stSize,表示需要的空间大小
        //这也是说明为什么stSize既是一个输入量也是一个输出量
        //释放原来的内存空间
        //重新申请内存空间用来存储所有网卡信息
        pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[stSize];
        if( ! pIpAdapterInfo )
            return -1;

        //再次调用GetAdaptersInfo函数,填充pIpAdapterInfo指针变量
        nRel=GetAdaptersInfo(pIpAdapterInfo,&stSize);    
    }

    if (ERROR_SUCCESS != nRel)
    {
        if( pIpAdapterInfo != &tTmpAdapter )
            delete[] pIpAdapterInfo;
        return -1;
    }

    //输出网卡信息
    //可能有多网卡,因此通过循环去判断
    while (pIpAdapterInfo && find == 0)
    {
        //  只记录以太网卡信息
        if( pIpAdapterInfo->Type != MIB_IF_TYPE_ETHERNET )
        {
            pIpAdapterInfo = pIpAdapterInfo->Next;
            continue;
        }

        IP_ADDR_STRING *pIpAddrString =&(pIpAdapterInfo->IpAddressList);

        //  只记录第一个 IP 地址
        while( pIpAddrString ){
            find        = 1;
            strcpy(strIP,pIpAddrString->IpAddress.String);
            break;
        }
    }

    //释放内存空间
    if( pIpAdapterInfo != &tTmpAdapter )
        delete[] pIpAdapterInfo;

    if( !find ){
        return -1;
    }
    return 0;
}


