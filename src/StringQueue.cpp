#include "../include/StringQueue.h"
#include <iostream>

std::string StringQueue::pop()
{
  //std::cout << "trying to pop msg\n";
  localMutex.lock();
  if (strings.empty()){
    //std::cout << "noMsg\n";
    localMutex.unlock();
    return "";
  }else{
    //std::cout << "msg found:\n";
    std::string *val = strings.front();
    //std::cout << val->c_str() << std::endl;
    strings.pop();
    elements--;
    localMutex.unlock();
    return std::string(val->c_str());
  }
}

void StringQueue::push(std::string* item)
{
  localMutex.lock();
  strings.push(item);
  elements++;
  localMutex.unlock();
}

int StringQueue::getElements(){
  localMutex.lock();
  int s = elements;
  localMutex.unlock();
  return s;
}
