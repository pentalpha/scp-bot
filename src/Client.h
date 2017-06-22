#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <cstdio>       //printf
#include <cstring>      //memset
#include <cstdlib>      //exit
#include <netinet/in.h> //htons
#include <arpa/inet.h>  //inet_addr
#include <sys/socket.h> //socket
#include <unistd.h>     //close
#include <string>
#include "logging.h"
 
#define MAXMSG 1024

class Client{
public:
    Client(std::string hostAddr, int portN);

    /*
    * Configurações do endereço
    */
    void configAddr();
    
    /*Criando o Socket
    * PARAM1: AF_INET ou AF_INET6 (IPV4 ou IPV6)
    * PARAM2: SOCK_STREAM ou SOCK_DGRAM
    * PARAM3: protocolo (IP, UDP, TCP, etc). Valor 0 escolhe automaticamente*/
    void createSocket();

    //Conectando o socket cliente ao socket servidor
    void connectToHost();

    //Enviar uma msg do cliente que se conectou
    void sendAMsg();

    //close client's socket
    void closeSocket();

private:
    struct sockaddr_in addr;
    int socketId;
    int hostPort;
    std::string hostAddress;
};

#endif