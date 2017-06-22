#include "Socket.h"

Socket::Socket(std::string hostAddr, int portN)
    : hostPort(portN), hostAddress(hostAddr)
{
    
    connected = false;
    receiving = false;
    exitFlag = false;
    if(!config()){
        exitFlag = true;
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
        log("SOCKET", "Falha ao executar socket()");
        return false;
    }
    return true;
}

bool Socket::isConnected(){
    return !exitFlag || connected;
}

void Socket::waitToFinish(){
    while(isConnected()){
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void Socket::finish(){
    exitFlag = true;
    connected = false;
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
    while(!exitFlag && connected){
        log("SOCKET", "SOCKET waiting for a message...");
        bytesread = recv(targetId,msg,MAXMSG,0);
        if (bytesread == -1)
        {
            log("SOCKET", "Failed to recv()");
            exitFlag = true;
            break;
        }
        else if (bytesread == 0)
        {
            log("SOCKET", "Client finished connection");
            exitFlag = true;
            break;
        }else{
            //Inserir o caracter de fim de mensagem
            msg[bytesread] = '\0';
            log("SOCKET", std::string("Servidor recebeu a seguinte msg do cliente: ") 
                + std::string(msg));
            std::string *s = new std::string(msg);
            received.push(s);
        }
        //
    }
    if(targetId != socketId){
        close(targetId);
    }
    finish();
}

bool Socket::sendAMsg(std::string msg, int targetId){
    if(targetId == -1){
        targetId = socketId;
    }
    std::string str = msg;
    int bytesenviados;
    //log("SOCKET", "SOCKETe vai enviar uma mensagem");
    //recv(connectionSOCKETId,msg,MAXMSG,0);
    bytesenviados = send(targetId,str.c_str(),strlen(str.c_str()),0);

    if (bytesenviados == -1)
    {
        log("SOCKET", "Falha ao executar send()");
        return false;
    }
    log("SOCKET", std::string("SOCKET enviou a seguinte msg (") + std::to_string(bytesenviados) 
        + std::string(" bytes) para o servidor:") + str);
    return true;
}