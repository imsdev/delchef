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

#define BACKLOG 10 // make bigger if this causes problems when deploying to labs
#define PORT "62085"
#define LOG_FILE "/var/log/delete-connecting-clients.log"
// This program assumes that knife is properly configured
// and has administrator access.  This will delete any
// client who connects through reverse DNS.

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
    FILE *log_file;
    log_file = fopen(LOG_FILE, "w");
    if (log_file == NULL){
        fprintf(stderr, "Log file could not be opened!\n");
        exit(1);
    }
    struct sockaddr_in cli_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res;
    int sockfd, new_fd;

    // !! don't forget your error checking for these calls !!

    // first, load up address structs with getaddrinfo():

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;  // use IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me
    
    if (getaddrinfo(NULL, PORT, &hints, &res) != 0){
        perror("getaddrinfo error");
        exit(1);
    }

    if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
        perror("Error creating socket");
        exit(1);
    }

    if (bind(sockfd, res->ai_addr, res->ai_addrlen) == -1){
        close(sockfd);
        perror("Error binding socket");
        exit(1);
    }
    freeaddrinfo(res); // free memory now that it is no longer in use
    // allow 5 backlog connections, this service should be used pretty irregularly
    if (listen(sockfd, BACKLOG) == -1){
        close(sockfd);
        perror("Error listening");
        exit(1);
    }

    addr_size = sizeof(cli_addr);
    char name[256];
    char ip[INET_ADDRSTRLEN]; 
    while(1){
        if (new_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &addr_size) == -1){
            perror("Error accepting connection");
            continue;
        }
        inet_ntop(AF_INET, &(cli_addr.sin_addr),ip,INET_ADDRSTRLEN);
        close(new_fd);
        printf("%s connected...\n", ip);
        
        getnameinfo((struct sockaddr*)&cli_addr, sizeof(cli_addr), name, sizeof(name), NULL, 0, 0);
        char comm[128];
        memset(comm,0,sizeof(comm));
        strcpy(comm,"knife client delete ");
        strcat(comm,name);
        strcat(comm, " -y");
        system(comm);
        fprintf(log_file, "Deleted: %s\n", name);
        fflush(log_file);
        sleep(1);
    }

}