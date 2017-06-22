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
#include <chrono>
#include <thread>
#include "logging.h"
#include "StringQueue.h"
#include "Socket.h"

class Client : public Socket{
public:
    Client(std::string hostAddr, int portN);
    bool startTransaction();
private:
    //Conectando o socket cliente ao socket servidor
    bool connectToHost(); 
};

#endif