
#if 0
#include <sys/socket.h>
#include <sys/types.h>
#include <resolv.h>
#include <fcntl.h>
#include <netdb.h>
#endif

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
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <resolv.h>
#include <netdb.h>      /* for getservbyname */
#include <ctype.h>      /* for isdigit */

#include "common.h"

#define MODULE_NAME "MAIN"
#define DEBUG_KEY   1 /* 0 means the debug message is disabled */



/*******************************************************************************
* Local compile flags
*******************************************************************************/


/*****************************************************************************
** Constant definitions
*****************************************************************************/


/*****************************************************************************
** Type definitions
*****************************************************************************/


/*****************************************************************************
** Macros
*****************************************************************************/


/*****************************************************************************
** Local variables
*****************************************************************************/


/*****************************************************************************
** Global variables
*****************************************************************************/


/*****************************************************************************
** Function prototypes
*****************************************************************************/


/*****************************************************************************
** Function bodies
*****************************************************************************/



/* servlet thread - get & convert the data */
void *tserver_thread(void *arg)
{
    int client = (int)arg;
    int readnum;
    int writenum;
    char s[100];

    PRINTF_DBG(RUNNING_LEVEL, "TCP client thread created, thread ID[%lu]\n", pthread_self());

    /* process client's requests */
    memset(s, '\0', 100);
    while ((readnum = (read(client, s, 100)) ) > 0)
    {
        if (s[0] == 'Q')
        {
            break;
        }

        PRINTF_DBG(DEBUG_LEVEL, "msg: %s\n", s);
        writenum = write(client, s, readnum);
        PRINTF_DBG(DEBUG_LEVEL, "writenum = %d\n", writenum);
        
        memset(s,'\0', readnum);
    }

    close(client);
    pthread_exit(0);          /* terminate the thread */
}


int main(int argv, char *args[])
{
    int ret;
    struct sockaddr_in addr;
    int sockfd;
    int acceptSockfd;
    int port;
    pthread_t child;
    int on = 1;
    pthread_attr_t pattr;

    if (argv != 2)
    {
        printf("usage: %s <protocol or portnum>\n", args[0]);
        exit(0);
    }

    signal(SIGPIPE, SIG_IGN);   //---ignore the SIGPIPE signal which is caused by writing a closed socket

    /* Get server's IP and standard service connection */
    if (!isdigit(args[1][0]))
    {
        struct servent *srv = getservbyname(args[1], "tcp");
        if ( srv == NULL )
        {
            ASSERT(0, args[1]);
        }
        PRINTF_DBG(RUNNING_LEVEL, "%s: port=%d\n", srv->s_name, ntohs(srv->s_port));
        port = srv->s_port;
    } 
    else
    {
        port = htons(atoi(args[1]));
    }

    /* create socket */
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        PRINTF_DBG(DEBUG_LEVEL, "socket\n");
        ASSERT(0, strerror(errno));
    }

    if ((setsockopt(sockfd, SOL_SOCKET,  SO_REUSEADDR, (char *)&on, sizeof(on)) ) < 0 )
    {
        PRINTF_DBG(DEBUG_LEVEL, "setsockopt\n");
        ASSERT(0, strerror(errno));
    }

    /* bind port/address to socket */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = port;
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd,(struct sockaddr*)&addr, sizeof(addr)) != 0)
    {
        PRINTF_DBG(DEBUG_LEVEL, "bind\n");
        ASSERT(0, strerror(errno));
    }

    /* set size of request queue to 10 */
    if (listen(sockfd, 10) < 0)
    {
        PRINTF_DBG(DEBUG_LEVEL, "listen\n");
        ASSERT(0, strerror(errno));
    }

    /* set thread create attributes */
    pthread_attr_init(&pattr);
    pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);

    while (1) 
    {
        PRINTF_DBG(RUNNING_LEVEL, "Waiting for a new connection ....\n");
        acceptSockfd = accept(sockfd, NULL, NULL);
        if (acceptSockfd < 0)
        {
            PRINTF_DBG(DEBUG_LEVEL, "accept\n");
            ASSERT(0, strerror(errno));
        }

        PRINTF_DBG(RUNNING_LEVEL, "New connection accepted on socket: %d\n", acceptSockfd);
        ret = pthread_create(&child, &pattr, tserver_thread, (void *)acceptSockfd);
        if (ret != 0) 
        {
            PRINTF_DBG(ERROR_LEVEL, "Create child failed, %s\n", strerror(errno));
            exit(1);
        }
    }


    close(sockfd);
    exit(0);
}



