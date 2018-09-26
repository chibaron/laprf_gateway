#include <stdio.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <monitor.h>
#include <laprf.h>

#define LAPRF_PORT   5403
#define MONITOR_PORT   5404
#define SERIAL_PORT "/dev/ttyACM0"

char buff[256];
char msg_buf[2048];

int open_serial(void)
{
  struct termios tio;                 // シリアル通信設定
  int baudRate = B9600;
  int fd;

    fd = open(SERIAL_PORT, O_RDWR);     // デバイスをオープンする
    if (fd < 0) {
        printf("serial open error\n");
        return -1;
    }

    tio.c_cflag += CREAD;               // 受信有効
    tio.c_cflag += CLOCAL;              // ローカルライン（モデム制御なし）
    tio.c_cflag += CS8;                 // データビット:8bit
    tio.c_cflag += 0;                   // ストップビット:1bit
    tio.c_cflag += 0;                   // パリティ:None

    cfsetispeed( &tio, baudRate );
    cfsetospeed( &tio, baudRate );
    cfmakeraw(&tio);                    // RAWモード
    tcsetattr( fd, TCSANOW, &tio );     // デバイスに設定を行う
    ioctl(fd, TCSETS, &tio);            // ポートの設定を有効にする

    return fd;
}

int main(int argc, char* argv[]) 
{

    int servSock; //server socket descripter
    int clitSock; //client socket descripter
    struct sockaddr_in servSockAddr; //server internet socket address
    struct sockaddr_in clitSockAddr; //client internet socket address
    unsigned int clitLen; // client internet socket address length
    int opt = 1;
    int len_net, len_tty;

    int fd_tty;                             // ファイルディスクリプタ

    monitor_init(MONITOR_PORT);

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
    servSockAddr.sin_port        = htons(LAPRF_PORT);

    if (bind(servSock, (struct sockaddr *) &servSockAddr, sizeof(servSockAddr) ) < 0 ) {
        perror("bind() failed.");
        exit(EXIT_FAILURE);
    }

    if (listen(servSock, 1) < 0) {
        perror("listen() failed.");
        exit(EXIT_FAILURE);
    }

    while(1) {
        printf("listen\n");
        clitLen = sizeof(clitSockAddr);
        if ((clitSock = accept(servSock, (struct sockaddr *) &clitSockAddr, &clitLen)) < 0) {
            perror("accept() failed.");
            exit(EXIT_FAILURE);
        }
        printf("connected from %s.\n", inet_ntoa(clitSockAddr.sin_addr));
		opt = 1;
		ioctl(clitSock, FIONBIO, &opt);		/* set non block */
		opt = 1;
        setsockopt(clitSock, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
        opt = 1;
        setsockopt(clitSock, IPPROTO_TCP, TCP_KEEPIDLE, &opt, sizeof(opt));
        opt = 1;
        setsockopt(clitSock, IPPROTO_TCP, TCP_KEEPINTVL, &opt, sizeof(opt));
        opt = 30;
        setsockopt(clitSock, IPPROTO_TCP, TCP_KEEPCNT, &opt, sizeof(opt));
		opt = 30000;	/* 30sec*/
		setsockopt(servSock, IPPROTO_TCP, TCP_USER_TIMEOUT, (void *)&opt, sizeof(opt));

        if ( (fd_tty = open_serial()) >= 0){
            while(1){
                // network->ttyACM0
    			len_net = read(clitSock, buff, sizeof(buff));
    			if (len_net > 0){
                    // uart送信
//                    printf("TCP->TTY:%02x\n", ch);
                    write(fd_tty, buff, len_net);
    			}else if (len_net == 0){
    				break;		/* 切断検出 */
    			}else if (errno != EAGAIN){
    				break;
    			}
                // ttyACM0->network
                len_tty = read(fd_tty, &buff, sizeof(buff));
                if (len_tty > 0){
                    for(int x=0; x<len_tty; x++){
                        printf("%02x ", buff[x]);
                        if (laprf_data(buff[x], msg_buf) > 0){
                            printf("\n");
                            printf("%s", msg_buf);
                            monitor_write(msg_buf);
                        }
                    }
           			write(clitSock, buff, len_tty);
    			}else if (len_tty == 0){
    				break;		/* 切断検出 */
    			}else if (errno != EAGAIN){
    				break;
    			}
                // データ通信がない場合は10msスリープ
                if(len_net<0 && len_tty<0){
                    usleep(10000);	/* 10ms */
    			}
            }
        }
        close(clitSock);
        close(fd_tty);
    }

    return EXIT_SUCCESS;
}
