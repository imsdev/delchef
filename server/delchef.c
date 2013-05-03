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
#include <time.h>

#define BACKLOG 10 // make bigger if this causes problems when deploying to labs
#define PORT "62085"
#define LOG_FILE "/var/log/delchef/delchef.log"
#define CLIENT_NAME "delchef"
#define KEY_LOCATION "/etc/chef/delchef.pem"

#define log(level, fmt, ...)              \
    logfile = fopen(LOG_FILE, "a");       \
    time_t ltime = time(NULL);            \
    struct tm* ctime = localtime(&ltime);\
    fprintf(logfile, "[%02d/%02d/%04d - %02d:%02d:%02d]: ", ctime->tm_mday, ctime->tm_mon, ctime->tm_year + 1900, ctime->tm_hour, ctime->tm_min, ctime->tm_sec); \
    fprintf(logfile, fmt, __VA_ARGS__);   \
    fprintf(logfile, "\n");               \
    fclose(logfile);                      \
    syslog(level, fmt, __VA_ARGS__);

// This program assumes that knife is properly configured
// and has administrator access.  This will delete any
// client who connects through DNS.

int main(int argc, char **argv){
    FILE *logfile = NULL;
    struct sockaddr_in cli_addr;
    socklen_t addr_size;
    struct addrinfo hints, *res, *p;
    int sockfd, new_fd;
    // first, load up address structs with getaddrinfo():
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;       // use IPv4
    hints.ai_socktype = SOCK_STREAM; // using TCP
    hints.ai_flags = AI_PASSIVE;     // fill in my IP for me, should be 0.0.0.0
    
    if (getaddrinfo(NULL, PORT, &hints, &res) != 0){
        log(LOG_ERR, "%s", "getaddrinfo() error");
        exit(1);
    }
    for (p = res; p != NULL; p = p->ai_next){
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1){
            log(LOG_ERR, "%s", "Error creating socket");
            continue;
        }
        int yes = 1;
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
            log(LOG_ERR, "%s", "Error settings socket options");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            log(LOG_ERR, "%s", "Error binding socket");
            continue;
        }

        break;    
    }
    if (p == NULL){
        close(sockfd);
        log(LOG_ERR, "%s", "Error binding socket");
        exit(1);
    }
    freeaddrinfo(res); // free memory now that it is no longer in use
    
    if (listen(sockfd, BACKLOG) == -1){
        close(sockfd);
        log(LOG_ERR, "%s", "Error listening");
        exit(1);
    }
    log(LOG_INFO, "%s", "Waiting for connections");
    addr_size = sizeof(cli_addr);
    char name[100];
    char ip[INET_ADDRSTRLEN]; 
    while(1){
        if (new_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &addr_size) == -1){
            log(LOG_ERR, "%s", "Error accepting connection");
            continue;
        }
        inet_ntop(AF_INET, &(cli_addr.sin_addr),ip,INET_ADDRSTRLEN);
        close(new_fd);
        getnameinfo((struct sockaddr*)&cli_addr, sizeof(cli_addr), name, sizeof(name), NULL, 0, 0);
        log(LOG_INFO, "Received connection from %s [%s]", ip, name);
        pid_t pid;
        pid = fork();
        if (pid == 0){ // child process
            int out = open(LOG_FILE, O_CREAT | O_RDWR | O_APPEND);
            if (out == -1){
                log(LOG_ERR, "%s", "Error opening log for child");
            }
            dup2(out, STDOUT_FILENO);
            dup2(out, STDERR_FILENO);
            close(out);
            execl("/usr/bin/knife", "/usr/bin/knife", "client", "delete", name, "-y", "-u", CLIENT_NAME, "-k", KEY_LOCATION, NULL);
            log(LOG_ERR, "%s", "Error executing execl()");
            exit(1);
        }
        else if (pid < 0){
            log(LOG_ERR, "%s", "Error forking a new process");
            exit(1);
        }
        sleep(1);
    }

}
