
#include "stdafx.h"

struct scanipctl{
    int         scan_ip;
    int         scan_mask;
    int         scan_port;

    LONG volatile scan_tskexitnum;
};


#define SET_SCANCTL(out,in){    \
    memset((out),0,sizeof(*(out))); \
    (out)->scan_ip = (in)->scan_ip; (out)->scan_mask = (in)->scan_mask; (out)->scan_port = (in)->scan_port;\
}


static DWORD WINAPI  tcpscan_thrproc(LPVOID  param )
{
    struct tskparam *pttskparam    = (struct tskparam *)param;
    struct scanipctl  *ptctl       = (struct scanipctl *)pttskparam->parent;

    unsigned int  ip       = 0;
    unsigned int  netmask = ((-1) << (32 - ptctl->scan_mask ));
    unsigned int  startip = (ntohl(ptctl->scan_ip) & netmask ) + (int)pttskparam->param * TCP_SCAN_NUM_1TSK;
    unsigned int  endip   = startip + TCP_SCAN_NUM_1TSK - 1;

    if( (startip & netmask ) != ( ntohl(ptctl->scan_ip) & netmask ) ){
        printf("SCAN TASK %d exit,not need to run\n",(int)param);
        return 0;
    }
    if( (startip & netmask ) != ( endip & netmask ) ){
        endip   = (startip & netmask ) | (( (1) <<(32 - ptctl->scan_mask ) )-1 );
    }

    /*
       printf("%d SCAN TSK SCAN IP %d.%d.%d.%d - %d.%d.%d.%d \n",
       (int)param,startip >> 24, (startip >> 16) & 0xff, (startip >> 8) & 0xff ,  (startip ) & 0xff,
       endip >> 24, (endip >> 16) & 0xff, (endip >> 8) & 0xff ,  (endip ) & 0xff
       );
       */
    for( ip = startip ; ip  <= endip ; ip++ ){
        struct in_addr      addr;
        char                ipstr[32]   = "";
        sprintf(ipstr,"%d.%d.%d.%d",ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff ,  (ip ) & 0xff);
        addr.s_addr = htonl(ip);
        strcpy(ipstr,inet_ntoa(addr));
        int sd  = os_tcp_connect_to(ipstr,ptctl->scan_port);
        if( sd <= 0  ){
            //printf("%s port %d CLOSE\n",ipstr,ptctl->scan_port);
        } else {
            printf("%s port %d OPEN +++++\n",ipstr,ptctl->scan_port);
            os_tcp_close(sd);
        }
    }

    printf("%02d SCAN TSK SCAN IP %d.%d.%d.%d - %d.%d.%d.%d FINISH\n",
            (int)param,startip >> 24, (startip >> 16) & 0xff, (startip >> 8) & 0xff ,  (startip ) & 0xff,
            endip >> 24, (endip >> 16) & 0xff, (endip >> 8) & 0xff ,  (endip ) & 0xff
          );

    InterlockedIncrement(&ptctl->scan_tskexitnum);
    return 0;
}




int run_scanip(struct proxyargs *ptargs)
{
    struct scanipctl tscanctl;
    int i   = 0;
    int tsknum = (1 << (32 - ptargs->scan_mask ))/TCP_SCAN_NUM_1TSK +1;
    struct tskparam *ptparam    = (struct tskparam *)malloc(tsknum * sizeof(struct tskparam ));

    if( !ptparam ){
        printf("alloc mem fail,stop\n");
        return -1;
    }

    SET_SCANCTL(&tscanctl,ptargs);

    tscanctl.scan_tskexitnum  = 0;
    printf("=======  BEGIN CREATE %02d SCAN TSK ========\n",tsknum);
    for( ; i < tsknum ; i ++ ){
        ptparam[i].parent   = &tscanctl;
        ptparam[i].param    = i;

        os_startthr(tcpscan_thrproc,(void *)(&ptparam[i]));
    }

    do{
        Sleep(3);
    }while( tscanctl.scan_tskexitnum != tsknum );

    free(ptparam);

    printf("SCAN FINISH. PRESS KEY TO EXIT\n");
    getchar();
    return 0;
}

