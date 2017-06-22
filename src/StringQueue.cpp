#include "StringQueue.h"

std::string StringQueue::pop()
{
  localMutex.lock();
  if (strings.empty())
  {
    return "";
  }
  std::string *val = strings.front();
  strings.pop();
  elements--;
  localMutex.unlock();
  return std::string(val->c_str());
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
