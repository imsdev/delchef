#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <syslog.h>
#include <signal.h>

#define BACKLOG 10 // make bigger if this causes problems when deploying to labs
#define PORT "62085"
// This program assumes that knife is properly configured
// and has administrator access.  This will delete any
// client who connects through DNS.

void daemonize()
{
    int i;
 
    if(getppid()==1) return; /* already a daemon */
    i=fork();
    if (i<0) exit(1); /* fork error */
    if (i>0) exit(0); /* parent exits */
 
    /* child (daemon) continues */
    setsid(); /* obtain a new process group */
    for (i=getdtablesize();i>=0;--i) close(i); /* close all descriptors */
    i=open("/dev/null",O_RDWR); dup(i); dup(i); /* handle standard I/O */
 
    /* first instance continues */
    signal(SIGCHLD,SIG_IGN); /* ignore child */
    signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);
}

int main(int argc, char **argv){
    daemonize();

    struct sockaddr_in cli_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res, *p;
    int sockfd, new_fd;

    // !! don't forget your error checking for these calls !!

    // first, load up address structs with getaddrinfo():

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // use IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
    
    if (getaddrinfo(NULL, PORT, &hints, &res) != 0){
        syslog(LOG_ERR, "getaddrinfo() error");
        exit(1);
    }
    for (p = res; p != NULL; p = p->ai_next){
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            syslog(LOG_ERR, "Error creating socket");
            continue;
        }
        int yes = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
            syslog(LOG_ERR, "Error settings socket options");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            syslog(LOG_ERR, "Error binding socket");
            continue;
        }

        break;    
    }
    if (p == NULL){
        close(sockfd);
        syslog(LOG_ERR, "Error binding socket");
        exit(1);
    }
    freeaddrinfo(res); // free memory now that it is no longer in use
    // allow 5 backlog connections, this service should be used pretty irregularly
    if (listen(sockfd, BACKLOG) == -1){
        close(sockfd);
        syslog(LOG_ERR, "Error listening");
        exit(1);
    }
    addr_size = sizeof(cli_addr);
    char name[100];
    char ip[INET_ADDRSTRLEN]; 
    while(1){
        if (new_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &addr_size) == -1){
            syslog(LOG_ERR, "Error accepting connection");
            continue;
        }
        inet_ntop(AF_INET, &(cli_addr.sin_addr),ip,INET_ADDRSTRLEN);
        close(new_fd);
        getnameinfo((struct sockaddr*)&cli_addr, sizeof(cli_addr), name, sizeof(name), NULL, 0, 0);
        char comm[128];
        memset(comm,0,sizeof(comm));
        strcpy(comm,"knife client delete ");
        strcat(comm,name);
        strcat(comm, " -y");
        system(comm);
        syslog(LOG_INFO, "Deleted: %s", name);
        sleep(1);
    }

}
