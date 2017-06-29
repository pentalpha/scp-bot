#include "Server.h"

Server::Server(const char* localIP, int portNumber)
    : Socket(localIP, portNumber)
{
    sizeAddressClient = sizeof(struct sockaddr);

    waitingFlag = false;
    bindedFlag = doBind();
    if(bindedFlag == false){
        exitFlag = true;
    }
}

bool Server::doBind(){
    //Conectando o socket a uma porta. Executado apenas no lado servidor
    if (bind (socketId, (struct sockaddr *)&addr, sizeof(struct sockaddr)) == -1)
    {
        error("SERVER", "Failed to bind() to a port");
        return false;
    }else{
        return true;
    }
}

bool Server::startListening(){
  //Habilitando o servidor a receber conexoes do cliente
  if (listen( socketId, 10 ) == -1)
  {
      error("SERVER", "Failed to listen() for upcoming connections");
      return false;
  }else{
      return true;
  }
}

int Server::waitForClient(){
    if(!bindedFlag){
        return -1;
    }
    if(!startListening()){
        return -1;
    }
    waitingFlag = true;
    //Servidor fica bloqueado esperando uma conexão do cliente
    //memset(&addressClient, 0, sizeAddressClient);
    int remoteClientId = accept(socketId, (struct sockaddr *) &addressClient, &sizeAddressClient);

    log(4,"SERVER", std::string("Connected to ") + std::string(inet_ntoa(addressClient.sin_addr)));
    waitingFlag = false;
    
    //Verificando erros
    if (remoteClientId == -1)
    {
        error("SERVER", std::string("Failed to accept(), error: ") + std::to_string(errno));
        exitFlag = true;
        return -1;
    }
    char ipStr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(addressClient.sin_addr), ipStr, INET_ADDRSTRLEN);
    clientAddress = std::string(ipStr);
    connected = true;
    asleep = false;

    
    return remoteClientId;
}

bool Server::startTransaction(){
    if(exitFlag || !isBinded()){
        return false;
    }
    connectionClientId = waitForClient();
    if(connectionClientId != -1){
        startReceiving(connectionClientId);
        return true;
    }
    return false;
}

bool Server::sendMsg(std::string str){
    return sendAMsg(str, connectionClientId);
}

bool Server::isBinded(){
    return bindedFlag;
}

bool Server::isWaiting(){
    return waitingFlag;
}


