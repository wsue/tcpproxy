
#ifndef VCPROXY_H_
#define VCPROXY_H_

//  TCP SCAN 配置参数
#define TCP_SCAN_TIMEOUT        15
#define TCP_SCAN_NUM_1TSK       10

//  TCP 代理配置参数
#define FORWARDBUF_MAXSZ        (4096*80)           /*  320k 大小的缓冲区   */

#define PROXY_PORT_CNT          4
#define DEF_PROXY_IP            "10.88.138.8"

//  参数参数
struct proxyargs{
    char	localip[32];
    char	proxyip[32];

    int         scan_ip;
    int         scan_mask;
    int         scan_port;
};

//  默认的任务参数
struct tskparam{
    void *parent;
    unsigned long param;
};

//  参数分析
int init_args(struct proxyargs *ptarg,int argc, _TCHAR* argv[]);
//  扫描IP
int run_scanip(struct proxyargs *ptargs);
int run_tcpproxy(const char *strlocalip,const char *proxyip);


//  底层接口
int os_tcp_listen(const char *ipaddr,int port);
int os_tcp_connect(const char *ipaddr,int port);
int os_tcp_connect_to(const char *ipaddr,int port,int conn_to = TCP_SCAN_TIMEOUT);
int os_tcp_close(int sd);
int os_writen(int sd,char *buf,int size);
int os_startthr( LPTHREAD_START_ROUTINE func,void *param);
int os_pcapsock_getselfip(char *strIP);
  

extern int g_proxy_ports[PROXY_PORT_CNT];

#endif


