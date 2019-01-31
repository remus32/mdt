#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

struct notifier {
  std::mutex m;
  std::condition_variable cv;

  int value;
  bool ready = 0;

  inline void notify(int i) {
    {
      std::lock_guard<std::mutex> lk(m);
      value = i;
      ready = 1;
    }
    cv.notify_one();
  };
  inline int wait() {
    std::unique_lock<std::mutex> lk(m);
    while(!ready) cv.wait(lk);
    ready = 0;
    return value;
  };
};
