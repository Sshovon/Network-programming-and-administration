#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define BACK_LOG 10

void error(const char *msg){
    perror(msg);
    exit(1);
}


int main(){

    FILE *html_data;
    html_data = fopen("index.html","r");
    
    char buffer[128];
    char response_data[2048]={'\0'};
    while(fgets(buffer,sizeof(buffer),html_data)){
        strcat(response_data,buffer);
    }

    char http_header[4096]="HTTP/1.1 200 OK\r\n\n";
    strcat(http_header,response_data);
    
    int server_socket;
    server_socket = socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8001);
    server_address.sin_addr.s_addr = INADDR_ANY;


    bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));

    listen(server_socket,BACK_LOG);
    int client_socket;
    while(1){
        client_socket= accept(server_socket,NULL, NULL); 
        send(client_socket,http_header,sizeof(http_header),0);
        close(client_socket);

    }

    return 0;
}