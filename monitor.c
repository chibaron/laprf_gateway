#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <termios.h>
#include <laprf.h>

#define ERROR_LOG_PRINTF    printf
#if 0
	#define DEBUG_LOG_PRINTF    printf
#else
	#define DEBUG_LOG_PRINTF(...)
#endif

static int port_no;
static bool sock_act = false;
static int clitSock; //client socket descripter

//
// モニタポートスレッド
//
void *monitor_thread(void *thdata)
{
    int servSock; //server socket descripter
    struct sockaddr_in servSockAddr; //server internet socket address
    struct sockaddr_in clitSockAddr; //client internet socket address
    unsigned int clitLen; // client internet socket address length
    int opt = 1;

    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 ){
        perror("socket() failed.");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt))){
        perror("setsockopt() failed.");
        exit(EXIT_FAILURE);
    }
    opt = 1;
    if (setsockopt(servSock, IPPROTO_TCP, TCP_NODELAY, (const char *)&opt, sizeof(opt))){
        perror("setsockopt() failed.");
        exit(EXIT_FAILURE);
    }
 
    memset(&servSockAddr, 0, sizeof(servSockAddr));
    servSockAddr.sin_family      = AF_INET;
    servSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servSockAddr.sin_port        = htons(port_no);

    if (bind(servSock, (struct sockaddr *) &servSockAddr, sizeof(servSockAddr) ) < 0 ) {
        perror("bind() failed.");
        exit(EXIT_FAILURE);
    }

    if (listen(servSock, 1) < 0) {
        perror("listen() failed.");
        exit(EXIT_FAILURE);
    }

    while(1) {
        printf("monitor listen\n");
        clitLen = sizeof(clitSockAddr);
        if ((clitSock = accept(servSock, (struct sockaddr *) &clitSockAddr, &clitLen)) < 0) {
            perror("accept() failed.");
            clitSock = -1;
            exit(EXIT_FAILURE);
        }
        printf("connected from %s.\n", inet_ntoa(clitSockAddr.sin_addr));
		opt = 1;
		ioctl(clitSock, FIONBIO, &opt);		/* set non block */
		opt = 1;
        setsockopt(clitSock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
        opt = 1;
        setsockopt(servSock, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
        opt = 1;
        setsockopt(clitSock, IPPROTO_TCP, TCP_KEEPIDLE, &opt, sizeof(opt));
        opt = 1;
        setsockopt(clitSock, IPPROTO_TCP, TCP_KEEPINTVL, &opt, sizeof(opt));
        opt = 30;
        setsockopt(clitSock, IPPROTO_TCP, TCP_KEEPCNT, &opt, sizeof(opt));
		opt = 30000;	/* 30sec*/
		setsockopt(servSock, IPPROTO_TCP, TCP_USER_TIMEOUT, (void *)&opt, sizeof(opt));

        sock_act = true;
        while(sock_act){
            sleep(1);
        }
        close(clitSock);
    }

    return NULL;
}

//
// 初期化
//
int monitor_init(int port)
{
 pthread_t thread;

    port_no = port;
	pthread_create( &thread, NULL, monitor_thread, (void *)NULL );
	pthread_detach(thread);

    return 0;
}

//
// データ出力
//
int monitor_write(char *msg)
{
    if (sock_act){
        if (0 >= write(clitSock, msg, strlen(msg) )){
            sock_act = false;
        }
    }
    return 0;
}

