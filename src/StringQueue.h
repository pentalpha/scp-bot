#ifndef _STR_CONCURRENT_QUEUE_
#define _STR_CONCURRENT_QUEUE_

#include <queue>
#include <thread>
#include <mutex>
#include <string>

class StringQueue
{
 public:
  std::string pop();
  void push(std::string* item);
  int getElements();
  StringQueue() : localMutex(){
      elements = 0;
  }

  StringQueue(const StringQueue&) = delete;            // disable copying
  StringQueue& operator=(const StringQueue&) = delete; // disable assignment
  int elements;
 private:
  std::queue<std::string*> strings;
  std::mutex localMutex;
};

#endif
