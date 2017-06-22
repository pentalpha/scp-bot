#include "Client.h"

Client::Client(std::string hostAddr, int portN)
    : hostPort(portN), hostAddress(hostAddr)
{
    configAddr();
    createSocket();
    connectToHost();
    sendAMsg();
    closeSocket();
}

void Client::configAddr(){
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(hostPort);
    addr.sin_addr.s_addr = inet_addr(hostAddress.c_str());
}

void Client::createSocket(){
    socketId = socket(AF_INET, SOCK_STREAM, 0);

    //Verificar erros
    if (socketId == -1)
    {
        log("CLIENT", "Falha ao executar socket()");
        exit(EXIT_FAILURE);
    }
}

void Client::connectToHost(){
    int connectRes = connect (socketId, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    if (connectRes == -1){
        log("CLIENT", "Falha ao executar connect()");
        exit(EXIT_FAILURE);
    }
    log("CLIENT", "Cliente conectado ao servidor");
}

void Client::sendAMsg(){
    std::string str = "Ola, servidor\nOla, servidor\nOla ola ola\nOla";
    int bytesenviados;
    log("CLIENT", "Cliente vai enviar uma mensagem");
    bytesenviados = send(socketId,str.c_str(),strlen(str.c_str()),0);

    if (bytesenviados == -1)
    {
        log("CLIENT", "Falha ao executar send()");
        exit(EXIT_FAILURE);
    }
    log("CLIENT", std::string("Cliente enviou a seguinte msg (") + std::to_string(bytesenviados) 
        + std::string(" bytes) para o servidor:") + str);
}


void Client::closeSocket(){
    close(socketId);
}