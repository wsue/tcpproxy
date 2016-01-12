
#include "stdafx.h"

static int parse_scanargs(struct proxyargs *ptarg,const char *argstr)
{
    char    ipstr[64]   = "";
    int     len         = strlen(argstr);
    char    *p          = NULL;
    if( len >= 64 ) {
        return -1;
    }

    strcpy(ipstr,argstr);

    p  = strchr(ipstr,':');
    if( p ) {
        ptarg->scan_port    = atoi(ipstr+1);
        *p  = 0;
    }

    p  = strchr(ipstr,'/');

    if( p ){
        ptarg->scan_mask    = atoi(p+1);
        *p  = 0;
    }

    ptarg->scan_ip      = inet_addr(ipstr);
    if( ptarg->scan_ip == 0 
            || ptarg->scan_mask < 1 || ptarg->scan_mask > 32
            ||ptarg->scan_port == 0 ){
        ptarg->scan_ip  = 0;
        return -1;
    }

    return 0;
}

int init_args(struct proxyargs *ptarg,int argc, _TCHAR* argv[])
{
    int i       = 1;

    memset(ptarg,0,sizeof(*ptarg));

    strcpy(ptarg->proxyip,DEF_PROXY_IP);
	ptarg->scan_port = 22; /* 默认扫描 ssh 端口 */
	
    if( argc <= 1 ){
        os_pcapsock_getselfip(ptarg->localip);
        return 0;
    }

    for( ; i < argc ; ) {
        if( !stricmp("-l",argv[i]) ){
            strcpy(ptarg->localip,argv[i+1]);
            i   += 2;
        } else  if( !stricmp("-p",argv[i]) ){
            strcpy(ptarg->proxyip,argv[i+1]);
            i   += 2;
        }
        else if( !stricmp("-s",argv[i]) ){
            if( parse_scanargs(ptarg,argv[i+1]) != 0 ){
                printf("wrong scan param ip:%x mask:%d port:%d\n",
                        ptarg->scan_ip,ptarg->scan_mask,ptarg->scan_port);
                return -1;
            }
            i   += 2;
        } else {
            return -1;
        }
    }

    return 0;
}


