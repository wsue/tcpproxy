
#include "stdafx.h"


struct tcpproxyctl{
    int     lsnsd;
    int     port;
    char    proxyip[32];
};

int     g_proxy_ports[PROXY_PORT_CNT]   = {80,  //  http
    443,                                        //  https
    9418                                        //  git port                              
};

static void tcppry_forward(int sd1, int sd2 )
{
    char    buf[FORWARDBUF_MAXSZ];
    int     sz;
    fd_set  rset; 
    int     fds[2]  = {sd1,sd2};
    int     i;


    int max = sd1 > sd2 ? sd1 +1 : sd2 +1;

    while ( 1 ) {
        FD_ZERO(&rset);
        FD_SET(sd1, &rset);
        FD_SET(sd2, &rset);

        /*      等待数据    */
        int ret = select(max,&rset,NULL,&rset,NULL);
        if( ret < 1 ) {
            return;
        }

        for( i = 0; i < 2; i ++ ) {
            if( FD_ISSET(fds[i], &rset) ) {
                /*  把数据从一个socket转发到另外一个socket  */
                sz  = recv(fds[i],buf,FORWARDBUF_MAXSZ,0);
                if ( sz > 0 ) {
                    if( os_writen(fds[(i+1)%2],buf,sz) == 0 )
                        sz      = 1;
                }

                if( sz <= 0 ) {
                    return ;
                }
            }
        }
    }

}

static DWORD WINAPI  tcptunnel(LPVOID  param )
{
    struct sockaddr_in  srcaddr;
    int           addrlen = sizeof(srcaddr);
    char    srcip[32]           = "";

    struct tskparam     *ptsockparam = (struct tskparam *)param;
    struct tcpproxyctl *ptctl =(struct tcpproxyctl *)ptsockparam->parent;
    int sd1  = (int )ptsockparam->param;
    int sd2 = os_tcp_connect(ptctl->proxyip,ptctl->port);
    memset(&srcaddr,0,sizeof(srcaddr));
    getpeername(sd1, (struct sockaddr *)&srcaddr, &addrlen);
    if( sd2 == -1 ) {
        printf("WARNING:<%d>    %s:%d(%d) => %s:%d create sd fail\n",
                ptctl->lsnsd,
                inet_ntoa( srcaddr.sin_addr),htons(srcaddr.sin_port),sd1,
                ptctl->proxyip,ptctl->port);
        os_tcp_close(sd1);
        return NULL;
    }


    tcppry_forward(sd1,sd2);
    printf("INFO:<%d>    %s:%d(%d) => %s:%d(%d) forward finish\n",
            ptctl->lsnsd,
            inet_ntoa( srcaddr.sin_addr),htons(srcaddr.sin_port),sd1,
            ptctl->proxyip,ptctl->port,sd2);
    Sleep(30);
    os_tcp_close(sd1);
    os_tcp_close(sd2);
    return NULL;
}



static DWORD WINAPI proxy_port_tsk(LPVOID param)
{
    struct tcpproxyctl *ptctl = (struct tcpproxyctl *)param;
    while( 1 ) {
        struct sockaddr_in  addr;
        int                 addrlen = sizeof(addr);
        struct tskparam     *ptsockparam = (struct tskparam     *)malloc(sizeof(struct tskparam));

        /* 接收新连接  */
        int     sd      = accept(ptctl->lsnsd,(sockaddr *)&addr,&addrlen);
        if( sd == -1 ) {
            printf("ERROR:<%d>      [port %d] accept new connect fail, exit\n",ptctl->lsnsd,ptctl->port);
            free(ptsockparam);
            return 1;
        }

        ptsockparam->parent = ptctl;
        ptsockparam->param  = (unsigned long)sd;
        if( os_startthr(forward_tsk,(void *)ptsockparam ) != 0 ){
            printf("WARNING:<%d>    [port %d] create parse thread fail\n",ptctl->lsnsd,ptctl->port);
            os_tcp_close(sd);
            free(ptsockparam);
        }
    }

    return 0;
}

int run_tcpproxy(const char *strlocalip,const char *proxyip)
{
    int     i           = 0;
    int     succnum     = 0;

    for( i = 0; i < PROXY_PORT_CNT && g_proxy_ports[i] != 0 ; i ++ ){
        int sd = os_tcp_listen(strlocalip,g_proxy_ports[i]);
        if( sd == -1 ){
            printf("ERROR: create listen socket fail\n");
        }
        else {
            struct tcpproxyctl  *ptctl  = (struct tcpproxyctl  *)malloc(sizeof(struct tcpproxyctl));
            if( ptctl ){
                memset(ptctl,0,sizeof(ptctl));
                ptctl->lsnsd    = sd;
                ptctl->port     = g_proxy_ports[i];
                strncpy(ptctl->proxyip,proxyip,sizeof(ptctl->proxyip)-1);
                if( os_startthr(proxy_port_tsk,(void *)ptctl ) == 0 ){
                    printf("run proxy for %s:%d SUCC\n",proxyip,g_proxy_ports[i]);
                    succnum ++;
                    continue;
                }

                printf("WARNING:    create parse thread fail\n");
            }
            else{
                printf("run proxy for %s:%d fail, no mem\n",proxyip,g_proxy_ports[i]);
            }

            if( ptctl ){
                free(ptctl);
            }
            os_tcp_close(sd);
        }
    }

    printf("SUCC RUN %d proxy\n",succnum);
    if( succnum > 0 ){
        do{
            Sleep(30);
        }while( 1);
    }

    return 0;
}

