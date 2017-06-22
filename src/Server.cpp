#include "Server.h"

Server::Server(const char* localIP, int portNumber){
  sizeAddressClient = sizeof(struct sockaddr);
  msg = new char[MAXMSG+1];
  /*
   * Configurações do endereço
  */
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_port = htons(portNumber);
  //address.sin_addr.s_addr = INADDR_ANY;
  address.sin_addr.s_addr = inet_addr(localIP);
  connected = false;
  exitFlag = false;
  waitingFlag =false;
}

bool Server::start(){
  if(getSocket() == false){
    return false;
  }
  if(doBind() == false){
    return false;
  }
  if(startListening() == false){
    return false;
  }
  waitingFlag = false;
  return true;
}

bool Server::getSocket(){
  /*
   * Criando o Socket
   *
   * PARAM1: AF_INET ou AF_INET6 (IPV4 ou IPV6)
   * PARAM2: SOCK_STREAM ou SOCK_DGRAM
   * PARAM3: protocolo (IP, UDP, TCP, etc). Valor 0 escolhe automaticamente
  */
  socketId = socket(AF_INET, SOCK_STREAM, 0);
  //Verificar erros
  if (socketId == -1)
  {   
      log("SERVER", "Failed to create socket()");
      return false;
  }
  return true;
}

bool Server::doBind(){
  //Conectando o socket a uma porta. Executado apenas no lado servidor
  if (bind (socketId, (struct sockaddr *)&address, sizeof(struct sockaddr)) == -1)
  {
    log("SERVER", "Failed to bind() a port");
    return false;
  }else{
    return true;
  }
}

bool Server::startListening(){
  //Habilitando o servidor a receber conexoes do cliente
  if (listen( socketId, 10 ) == -1)
  {
      log("SERVER", "Failed to listen()");
      return false;
  }else{
      return true;
  }
}

bool Server::isConnected(){
  return connected || !exitFlag;
}

string Server::getMessage(){
  string msg = messages.pop();
  if(msg == ""){
    return "";
  }else{
    return msg;
  }
}

void Server::startWaiting(){
  exitFlag = false;
  waitingFlag = true;
  thread theThread = thread(&Server::waitForClientAndReceive, this);
  theThread.join();
}

void Server::stop(){
  log("SERVER", "Server auto stopping itself");
  exitFlag = true;
}

bool Server::isWaiting(){
  return waitingFlag;
}

void Server::waitForClientAndReceive(){
  //servidor fica em um loop infinito
  log("SERVER", "Waiting for a client");

  waitingFlag = true;
  //Servidor fica bloqueado esperando uma conexão do cliente
  connectionClientId = accept( socketId,(struct sockaddr *) &addressClient,&sizeAddressClient );

  log("SERVER", std::string("Connected to ") + std::string(inet_ntoa(addressClient.sin_addr)));
  waitingFlag = false;
  //Verificando erros
  if (connectionClientId == -1)
  {
      log("SERVER", std::string("Failed to accept(), error: ") + std::to_string(errno));
      exitFlag = true;
      return;
  }
  connected = true;
  while(!exitFlag){
    //receber uma msg do cliente
    //log("SERVER", "Server waiting for a message...\n";
    bytesread = recv(connectionClientId,msg,MAXMSG,0);
    if (bytesread == -1)
    {
        log("SERVER", "Failed to recv()");
        break;
    }
    else if (bytesread == 0)
    {
        log("SERVER", "Client finished connection");
        exitFlag = true;
        break;
    }else{
        //Inserir o caracter de fim de mensagem
        msg[bytesread] = '\0';
        log("SERVER", std::string("Servidor recebeu a seguinte msg do cliente: ") 
            + std::string(msg));
        string *s = new string(msg);
        messages.push(s);
    }
    //close(connectionClientId);
  }
  waitingFlag = false;
  connected = false;
  close(connectionClientId);
}