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
#include <sys/time.h>

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
#define TX_INTERVAL_SEC     1       /* sec */
#define TX_INTERVAL_USEC    0       /* usec */
#define DIS_INTERVAL        2

/*****************************************************************************
** Type definitions
*****************************************************************************/
typedef enum
{
    TIMER_TX,
    TIMER_DIS,
    TIMER_MAX_NUM
} tc_timer_t;


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
static pthread_t    timerThread;

static char dstIpStr[20] = {0};
int sockfd;
struct sockaddr_in servaddr;
struct sockaddr_in cliaddr;
static struct timeval time_now;
static const struct timeval tx_ti = {TX_INTERVAL_SEC, TX_INTERVAL_USEC};
static struct timeval tx_to;

static const struct timeval dis_ti = {DIS_INTERVAL, 0};
static struct timeval dis_to;
static int timeToTx = 0;
static int timeToDis = 0;

static char *unknownStr = "Uknown type";
static char *tcTimerTypeStr[] =
{
    "TIMER_TX",
    "TIMER_DIS",
    "TIMER_UNKNOWN"
};


/*****************************************************************************
** Global variables
*****************************************************************************/


/*****************************************************************************
** Function prototypes
*****************************************************************************/


/*****************************************************************************
** Function bodies
*****************************************************************************/
static char *convertTimerToStr(tc_timer_t timerType)
{
    if (timerType >= TIMER_MAX_NUM)
    {
        return unknownStr;
    }
    
    return tcTimerTypeStr[timerType];
}


static void timerStart(tc_timer_t timerType)
{
    PRINTF_DBG(DEBUG_LEVEL, "%s\n", convertTimerToStr(timerType));

    switch (timerType)
    {
        case TIMER_TX:
            timeradd(&time_now, &tx_ti, &tx_to);
            break;

        case TIMER_DIS:
            timeradd(&time_now, &dis_ti, &dis_to);
            break;
            
        default:
            ASSERT(0, "Unknow timer");
            break;
    }
}


#if 0
static void timerStop(tc_timer_t timerType)
{
    PRINTF_DBG(DEBUG_LEVEL, "%s\n", convertTimerToStr(timerType));

    switch (timerType)
    {
        case TIMER_TX:
            timerclear(&tx_to);
            break;

        case TIMER_DIS:
            timerclear(&dis_to);
            break;

        default:
            ASSERT(0, "Unknow timer");
            break;
    }
}
#endif


static void timerCheck(void)
{
    /* TIMER_TX */
    if (timerisset(&tx_to) && timercmp(&time_now, &tx_to, >=))
    {
        timerclear(&tx_to);
        timerStart(TIMER_TX);
        timeToTx = 1;
    }

    /* TIMER_DIS */
    if (timerisset(&dis_to) && timercmp(&time_now, &dis_to, >=))
    {
        timerclear(&dis_to);
        timerStart(TIMER_DIS);
        timeToDis = 1;
    }
}


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


static int tclient_tx(void)
{
    static int i = 0;
    int writenum;
    char txData[TX_RX_BUFFER_SIZE] = {0};

    if (timeToTx)
    {
        timeToTx = 0;
   
        sprintf(txData, "This is a book, %d", i++);
        writenum = write(sockfd, txData, strlen(txData));
        if (writenum != strlen(txData))
        {
            PRINTF_DBG(DEBUG_LEVEL, "%s\n", strerror(errno));
            return -1;
        }
        
        PRINTF_DBG(DEBUG_LEVEL, "writenum = %d\n", writenum);
    }

    return 0;
}


static int tclient_rx(void)
{
    int readnum;
    char rxData[TX_RX_BUFFER_SIZE] = {0};

    readnum = recvfrom(sockfd, rxData, TX_RX_BUFFER_SIZE, 0, NULL, NULL);
    PRINTF_DBG(DEBUG_LEVEL, "readnum = %d, %s\n", readnum, rxData);
    if (readnum == 0) 
    {
        PRINTF_DBG(DEBUG_LEVEL, "client disconnected\n");
        return (-1);
    }

    return 0;
}


static int tclient_select(void)
{
    int ret;
    fd_set readfs;
    fd_set writefs;
    struct timeval to;

    if (0 >= sockfd)
    {
        return 0;
    }

    FD_ZERO(&readfs); 
    FD_SET(sockfd, &readfs);
    FD_ZERO(&writefs); 
    FD_SET(sockfd, &writefs);

    to.tv_sec = 1;
    to.tv_usec = 0;

    ret = select(sockfd + 1, &readfs, &writefs, NULL, &to);
    //PRINTF_DBG(RUNNING_LEVEL, "select return %d\n", ret);
    if (ret < 0)
    {
        PRINTF_DBG(RUNNING_LEVEL, "TCP client thread created, thread ID[%lu]\n", pthread_self());
    }
    else if (ret == 0)
    {
        PRINTF_DBG(RUNNING_LEVEL, "select timeout\n");
    }
    else
    {
        if (FD_ISSET(sockfd, &readfs))
        {
            if (-1 == tclient_rx())
            {
                return (-1);
            }
        }

        if (FD_ISSET(sockfd, &writefs))
        {
            if (-1 == tclient_tx())
            {
                return (-1);
            }
        }
    }

   return 0;
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
            else
            {
                timerStart(TIMER_TX);
                timerStart(TIMER_DIS);
            }
        }

        ret = tclient_select();
        if (-1 == ret)
        {
            cLOSEsOCKET(sockfd);
        }
        else if (timeToDis)
        {
            cLOSEsOCKET(sockfd);
            timeToDis = 0;
        }
    }

    return NULL;
}


static void *timer_thread(void * arg) 
{
    while (1)
    {
        gettimeofday(&time_now, NULL);

        timerCheck();
    }

    return NULL;
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

   ret = pthread_create(&timerThread, NULL, timer_thread, NULL);
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

    return 0;
}

