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
#define TCP_PORT            503
#define TX_RX_BUFFER_SIZE   32

/*****************************************************************************
** Type definitions
*****************************************************************************/


/*****************************************************************************
** Macros
*****************************************************************************/
#define cLOSEsOCKET(fd) \
do                      \
{                       \
    PRINTF_DBG(RUNNING_LEVEL, "close soceket\n");   \
    close(fd);                                      \
    fd = 0;                                         \
} while(0);


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
    
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);
    
    ret = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if ((ret == 0) || ((ret < 0) && (errno == 0 || errno == EINPROGRESS)))
    {
        PRINTF_DBG(DEBUG_LEVEL, "try to connect to Server %s:%d ... OK\n", dstIpStr, TCP_PORT);
        ret = 0;
    }
    else
    {
        PRINTF_DBG(DEBUG_LEVEL, "try to connect to Server %s:%d ... failed\n", dstIpStr, TCP_PORT);
        ret = (-1);
    }

    return ret;
}


static int tclient_loop(void)
{
#if 1
    if (0 >= sockfd)
    {
        return 0;
    }

    while(1)
    {
        int ret;
        static i = 0;
        int writenum;
        int readnum;
        char txData[TX_RX_BUFFER_SIZE] = {0};
        char rxData[TX_RX_BUFFER_SIZE] = {0};

        sprintf(txData, "This is a book, %d", i++);
        writenum = write(sockfd, txData, strlen(txData));
        if (writenum != strlen(txData))
        {
            PRINTF_DBG(DEBUG_LEVEL, "ret = %d, %s\n", ret, strerror(errno));
        }

        PRINTF_DBG(DEBUG_LEVEL, "writenum = %d\n", writenum);
#if 1
        readnum = recvfrom(sockfd, rxData, TX_RX_BUFFER_SIZE, 0, NULL, NULL);
        PRINTF_DBG(DEBUG_LEVEL, "readnum = %d, %s\n", readnum, rxData);
        if (readnum == 0) 
        {
            PRINTF_DBG(DEBUG_LEVEL, "client disconnected\n");
            return (-1);
        }
#else
        readnum = read(sockfd, rxData, TX_RX_BUFFER_SIZE);
        PRINTF_DBG(DEBUG_LEVEL, "readnum = %d, %s\n", readnum, rxData);
        if (0 < readnum)
        {
            PRINTF_DBG(DEBUG_LEVEL, "received data: %s\n", rxData);
        }
#endif

        sleep(1);
        //usleep(1000);

        return -1;
   }

   return 0;
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

    while (1)
    {
        if (!sockfd)
        {
            ret = tclient_init();
            if (0 > ret)
            {
                cLOSEsOCKET(sockfd);
            }
        }

        ret = tclient_loop();
        if (-1 == ret)
        {
            cLOSEsOCKET(sockfd);
        }
    }
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

