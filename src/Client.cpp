#include "Client.h"

Client::Client(std::string hostAddr, int portN)
    : Socket(hostAddr, portN)
{

}

bool Client::startTransaction(){
    if(connectToHost()){
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
        error("CLIENT", "Failed to connect()");
        return false;
    }
    connected = true;
    asleep = false;
    log(2,"CLIENT", "Client connected to server");
    return true;
}

bool Client::sendMsg(std::string str){
    return sendAMsg(str);
}

