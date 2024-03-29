#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#define port 80
#define back_log 10

void daemonize()
{
    pid_t pid; // pid =-1 error pid=0 child pid>0 parent
    pid = fork();

    if (pid < 0)
    {
        exit(EXIT_FAILURE); // error child process was  not successfully created
    }
    if (pid > 0)
    {
        exit(EXIT_SUCCESS); // successfully exited from parent process
    }
    if (setsid() < 0) // setsid()<0 means error
    {
        exit(EXIT_FAILURE); // exit with failure code
    }

    signal(SIGCHLD, SIG_IGN); // ignoring sigchld
    signal(SIGHUP, SIG_IGN);  // ignoring sighup

    pid = fork();

    if (pid > 0)
    {
        exit(EXIT_SUCCESS); // terminating parent process second time
    }
    chdir("/home/frcs/vscode/c++/Assignment"); // changing to root directory
    umask(0);   // changing permission
    close(0);   // closing file descriptor
    close(1);
}

int main()
{
    daemonize(); // daemonize the process

    int serverSocket, clientSocket; // for server and client socket
    int on = 1;                     // this is for setsockopt enable value
    struct sockaddr_in server_address; 

    char httpRequest[2048];
    char httpNotFound[] = "HTTP/1.1 404 Not Found\n\n404 Not Found\n";
    char httpUnsupportedMediaType[] = "HTTP/1.1 415 Unsupported Media Type\n\n415 Unsupported Media Type.\n";
    char httpResponse[512];
    char *filePath, *mime, c; // this will store the file path given by the client

    FILE *fp, *log; // these are file pointers

    log = fopen("/home/frcs/vscode/c++/error.log", "a"); // u have to give your own path ***** && delete this comment :p
    dup2(fileno(log), STDERR_FILENO);                    // this will redirect stderr messages to error.log file
    fclose(log);                                         // closing error.log filepointer

    if(chroot("/home/frcs/vscode/c++/Assignment")){
        fprintf(stderr,"Coudn't change root\n");
    }; // changing root directory for the current process and making this directory webroot

    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // ipv4, tcp , tcp protocol
    if (serverSocket < 0)
    {
        perror("socket creation failed!\n"); // error log for failed socked creation
    }

    server_address.sin_family = AF_INET;         // ipv4
    server_address.sin_addr.s_addr = INADDR_ANY; // local host
    server_address.sin_port = htons(port);       // defined port=80

    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)); // setting up options for server socket 

    if (bind(serverSocket, (struct sockaddr *)&server_address, sizeof(server_address)) == 0) // binding socket with server address
    {
        fprintf(stderr, "Prosess %d er running on port %d.\n", getpid(), port);
    }
    else
    {
        perror("Binding server socket failed");
        exit(2); // exiting process with major bug(2) signal
    }
    
    //drop the root privilleges and make you the process owner
    if (!getuid()) // if getuid returns 0 then it means  this process owner is a super user / root 
    {
        if (setgid(1000) != 0) // seting group id
            perror("setgid: Unable to drop group privileges.\n");
        if (setuid(1000) != 0) // setting user id
            perror("setuid: Unable to drop user privileges.\n");
    }
    else
    {
        fprintf(stderr, "not root!\n"); // owner is not root
    }

    listen(serverSocket, back_log); // listening to requests made by the client
    while (1)
    {
        clientSocket = accept(serverSocket, NULL, NULL); // accepting client
        if (clientSocket < 0)
        {
            perror("accept failed!!\n"); // failed to accept client socket.
        }
        recv(clientSocket, httpRequest, sizeof(httpRequest), 0); // received http request from client
        if (!fork())                                             // creating new process for each client request by creating a child process
        {
            dup2(clientSocket, 1);    // redirecting stdout to client socket
            fprintf(stderr,"%s\n",httpRequest); //newly added
            strtok(httpRequest, " "); // processing filepath from http reuqest
            filePath = strtok(NULL, " ");
            
            strtok(filePath,".");
            mime=strtok(NULL,".");

            if(strcmp(mime,"asis")){
                fprintf(stderr, "415 Unsupported Media Type.\n");                                      // log error in the error.log file
                write(clientSocket, httpUnsupportedMediaType, sizeof(httpUnsupportedMediaType) - 1); // sending error message to client
                close(clientSocket);
            }else{
                strcat(filePath,".asis");
                if(access(filePath,F_OK)){
                    fprintf(stderr, "404 Not Found\n");                                    
                    write(clientSocket, httpNotFound, sizeof(httpNotFound) - 1); 
                    close(clientSocket);
    
                }else{
                    fp=fopen(filePath,"r");
                    fprintf(stderr, "200 OK\n"); // log in the error.log file
                    while (1)
                    {
                        c = fgetc(fp);
                        if (feof(fp))
                        {
                            break;
                        }
                        write(clientSocket, &c, sizeof(c));
                    }
                    fclose(fp);
                }
                

            }

            fflush(stderr);
            shutdown(clientSocket, SHUT_RDWR);
            exit(0);
        }  
        else
        {
            signal(SIGCHLD, SIG_IGN); // ignoring zombie process
            close(clientSocket);
        }
    }

    close(serverSocket);
    return 0;
}

