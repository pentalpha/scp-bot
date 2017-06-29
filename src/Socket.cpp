#include "Socket.h"

Socket::Socket(std::string hostAddr, int portN)
    : hostPort(portN), hostAddress(hostAddr)
{
    asleep = true;
    connected = false;
    receiving = false;
    exitFlag = false;
    if(!config()){
        exitFlag = true;
    }else{

    }
}

bool Socket::config(){
    configAddr();
    return createSocket();
}

void Socket::configAddr(){
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(hostPort);
    addr.sin_addr.s_addr = inet_addr(hostAddress.c_str());
}

bool Socket::createSocket(){
    socketId = socket(AF_INET, SOCK_STREAM, 0);
    //Verificar erros
    if (socketId == -1)
    {
        error("SOCKET", "Failed to create POSIX socket()");
        return false;
    }
    return true;
}

bool Socket::isConnected(){
    return !exitFlag || connected;
}

bool Socket::isAsleep(){
    return asleep;
}

void Socket::waitToFinish(){
    while(isConnected()){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Socket::finish(){
    exitFlag = true;
    connected = false;
    asleep = true;
    closeSocket();
}

void Socket::closeSocket(){
    close(socketId);
}

std::string Socket::getMessage(){
  std::string msg = received.pop();
  return msg;
}

void Socket::startReceiving(int targetId){
    if(targetId == -1){
        targetId = socketId;
    }
    std::thread theThread(&Socket::receiveMessagesThread, this, targetId);
    theThread.join();
}

void Socket::receiveMessagesThread(int targetId){
    receiving = true;
    char* msg = new char[MAXMSG+1];
    int bytesread = 0;
    std::string buffer = "";
    while(!exitFlag && connected){
        log(0, "SOCKET", "SOCKET waiting for a message...");
        memset(msg, 0, sizeof(msg));
        bytesread = recv(targetId,msg,MAXMSG,0);
        if (bytesread == -1)
        {
            error("SOCKET", "Failed to do recv()");
            exitFlag = true;
            break;
        }
        else if (bytesread == 0)
        {
            log(4,"SOCKET", "Client finished connection");
            exitFlag = true;
            break;
        }else{
            //Inserir o caracter de fim de mensagem
            msg[bytesread] = '\0';
            std::string message(msg);
            log(0,"SOCKET", std::string("recv() = ") 
                + message);
            buffer = retrieveMessagesFromBufferAndRecv(buffer, message);
        }
        //
    }
    if(targetId != socketId){
        close(targetId);
    }
    finish();
}

std::string Socket::retrieveMessagesFromBufferAndRecv(std::string buffer, std::string message){
    buffer = buffer + message;
    log(0,"SOCKET", std::string("Buffer: ") + buffer);
    int i = 0;
    int lastLineBreak;
    int msgStart = 0;
    while(i < buffer.length()){
        if(buffer[i] == '\n'){
            lastLineBreak = i;
            std::string subMsg = buffer.substr(msgStart, lastLineBreak - msgStart);
            addMsgToReceiveds(subMsg);
            msgStart = i+1;
        }
        i++;
    }
    std::string bufferLeft = buffer.substr(msgStart, buffer.length());
    log(0,"SOCKET", std::string("Buffer left: ") + bufferLeft);
    return bufferLeft;
}

void Socket::addMsgToReceiveds(std::string message){
    std::string *s = new std::string(message);
    received.push(s);
    log(3,"SOCKET", std::string("Received: ") + message);
}

bool Socket::sendAMsg(std::string msg, int targetId){
    if(targetId == -1){
        targetId = socketId;
    }
    std::string str = msg;
    if(str[str.length()-1] != '\n'){
        str += "\n";
    }
    int bytesenviados;
    log(0, "SOCKET", "SOCKET is going to send a message");
    //recv(connectionSOCKETId,msg,MAXMSG,0);
    bytesenviados = send(targetId,str.c_str(),strlen(str.c_str()),0);

    if (bytesenviados == -1)
    {
        error("SOCKET", "Failed to do send()");
        return false;
    }
    log(3,"SOCKET", std::string("SOCKET sent: (") + std::to_string(bytesenviados) 
        + std::string(" bytes) to remote socket:") + str);
    return true;
}