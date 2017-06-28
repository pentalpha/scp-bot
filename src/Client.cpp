#include "Client.h"

Client::Client(std::string hostAddr, int portN)
    : Socket(hostAddr, portN)
{

}

bool Client::startTransaction(){
    if(connectToHost()){
        sendAMsg("Ola, servidor\nOla, servidor\nOla ola ola\nOla\n");
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
        log("CLIENT", "Failed to connect()");
        return false;
    }
    connected = true;
    asleep = false;
    log("CLIENT", "Client connected to server");
    return true;
}

bool Client::sendMsg(std::string str){
    return sendAMsg(str);
}

