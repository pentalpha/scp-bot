#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <cstdio>       //printf
#include <cstring>      //memset
#include <cstdlib>      //exit
#include <netinet/in.h> //htons
#include <arpa/inet.h>  //inet_addr
#include <sys/socket.h> //socket
#include <unistd.h>     //close
#include <string>
#include <chrono>
#include <thread>
#include "logging.h"
#include "StringQueue.h"
 
#define MAXMSG 7

class Socket{
public:
    Socket(std::string hostAddr, int portN);
    bool config();
    virtual bool startTransaction() = 0;
    virtual bool sendMsg(std::string msg) = 0;
    bool isAsleep();
    bool isConnected();
    void waitToFinish();
    void finish();
    std::string getMessage();
protected:
    /*
    * Configurações do endereço
    */
    void configAddr();
    
    /*Criando o Socket
    * PARAM1: AF_INET ou AF_INET6 (IPV4 ou IPV6)
    * PARAM2: SOCK_STREAM ou SOCK_DGRAM
    * PARAM3: protocolo (IP, UDP, TCP, etc). Valor 0 escolhe automaticamente*/
    bool createSocket();
    //Enviar uma msg
    bool sendAMsg(std::string msg, int targetId = -1);
    void startReceiving(int targetId = -1);
    
    
    //close client's socket
    void closeSocket();

    struct sockaddr_in addr;
    int socketId;
    int hostPort;
    std::string hostAddress;
    StringQueue received;
    bool asleep, connected, exitFlag, receiving;
private:
    std::string retrieveMessagesFromBufferAndRecv(std::string buffer, std::string message);
    void addMsgToReceiveds(std::string message);
    void receiveMessagesThread(int targetId);
};

#endif