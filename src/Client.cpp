#include "Client.h"

Client::Client(std::string hostAddr, int portN)
    : Socket(hostAddr, portN)
{

}

bool Client::startTransaction(){
    if(connectToHost()){
        sendAMsg("Ola, servidor\nOla, servidor\nOla ola ola\nOla");
        startReceiving();
        return true;
    }
    exitFlag = true;
    finish();
    return false;
}

bool Client::connectToHost(){
    int connectRes = connect (socketId, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    if (connectRes == -1){
        log("CLIENT", "Falha ao executar connect()");
        return false;
    }
    connected = true;
    log("CLIENT", "Cliente conectado ao servidor");
    return true;
}

