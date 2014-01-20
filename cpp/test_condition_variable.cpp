#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <random>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdint>
#include <queue>

using namespace std;
using namespace std::chrono;

condition_variable condition;
mutex mux;
bool go = false;

void client( string name )
{
  unique_lock<mutex> lock(mux);
  cout << name << " waiting" << endl;
  condition.wait(lock, [&](){ return go; });
  cout << name << "going" << endl;
}

void server()
{
  unique_lock<mutex> lock(mux);
  this_thread::sleep_for(milliseconds(200));
  cout << "...serving..." << endl;
  go = true;
  condition.notify_one();
  lock.unlock();
}

int main()
{
  thread t1(&client, string("one"));

  this_thread::sleep_for(milliseconds(100));

  thread s(&server);

  this_thread::sleep_for(milliseconds(100));

  thread t2(&client, string("two"));

  s.join();
  t1.join();
  t2.join();
}
