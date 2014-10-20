#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <asm/termios.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>         /* memset */
#include <signal.h>         /* Signal handling */
#include <sys/types.h>
#include <sys/syscall.h>    /* for syscall */
#include <time.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include "common.h"

#define MODULE_NAME "MAIN"
#define DEBUG_KEY   1 /* 0 means the debug message is disabled */



/*******************************************************************************
* Local compile flags
*******************************************************************************/


/*****************************************************************************
** Constant definitions
*****************************************************************************/
#define TCP_PORT        10555

/*****************************************************************************
** Type definitions
*****************************************************************************/


/*****************************************************************************
** Macros
*****************************************************************************/


/*****************************************************************************
** Local variables
*****************************************************************************/
static pthread_t    tClientThread;
static char dstIpStr[20] = {0};
int sockfd;
struct sockaddr_in servaddr;
struct sockaddr_in cliaddr;



/*****************************************************************************
** Global variables
*****************************************************************************/


/*****************************************************************************
** Function prototypes
*****************************************************************************/


/*****************************************************************************
** Function bodies
*****************************************************************************/


static int tclient_init(void)
{
    int ret;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(dstIpStr);
    servaddr.sin_port = htons(TCP_PORT);
    
    //fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);

    PRINTF_DBG(DEBUG_LEVEL, "try to connect to Server %s:%d\n", dstIpStr, TCP_PORT);
    ret = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if ((ret == 0) || ((ret < 0) && (errno == 0 || errno == EINPROGRESS)))
    {
        ret = 0;
    }
    else
    {
        ret = (-1);
    }

    return ret;
}


static void tclient_loop(void)
{
#if 1
    if (0 >= sockfd)
    {
        return;
    }

    while(1)
    {
        int ret;
        static i = 0;

        char txData[32] = {0};

        sprintf(txData, "This is a book, %d", i++);
        ret = write(sockfd, txData, strlen(txData));
        if (ret != strlen(txData))
        {
            PRINTF_DBG(DEBUG_LEVEL, "ret = %d, %s\n", ret, strerror(errno));
        }
        sleep(1);
   }
#else
        while (fgets(sendline, 10000, stdin) != NULL)
        {
            int n;   
            char sendline[1000];
            char recvline[1000];
    
            sendto(sockfd, sendline, strlen(sendline), 0, (struct sockaddr *)&servaddr, sizeof(servaddr));
    
            n = recvfrom(sockfd, recvline, 10000, 0, NULL, NULL);
            recvline[n] = 0;
            fputs(recvline,stdout);
       }
#endif

}


static void *tclient_thread(void * arg) 
{
    int ret;

    PRINTF_DBG(RUNNING_LEVEL, "TCP client thread created, thread ID[%lu]\n", pthread_self());

    ret = tclient_init();
    if (0 > ret)
    {
        PRINTF_DBG(RUNNING_LEVEL, "close soceket\n");
        close(sockfd);
        sockfd = 0;
    }

    tclient_loop();
}



int main(int argc, char**argv)
{
    int ret;

    signal(SIGPIPE, SIG_IGN);   //---ignore the SIGPIPE signal which is caused by writing a closed socket

    if (argc != 2)
    {
        printf("usage:  client <IP address>\n");
        exit(1);
    }

    ret = inet_addr(argv[1]);
    if (INADDR_NONE == ret)
    {
        printf("wrong parameter, %s\n", argv[1]);
        printf("usage:  client <IP address>\n");
        exit(1);
    }

    strcpy(dstIpStr, argv[1]);


   ret = pthread_create(&tClientThread, NULL, tclient_thread, NULL);
   if (ret != 0) 
   {
       PRINTF_DBG(ERROR_LEVEL, "Create tClientThread failed, %s\n", strerror(errno));
       exit(1);
   }

    ret = pthread_join(tClientThread, (void *)NULL);
    if (ret != 0) 
    {
        PRINTF_DBG(ERROR_LEVEL, "join tClientThread failed: %s\n", strerror(errno));

        /* this exit will cause all the threads to be terminated */
        exit(1);
    }   



}

