/* Header file for River Crossing Program */
#include <chrono>
#include <mutex>
#include <condition_variable>

#define TOTAL_PASSENGER_COUNT 1000

using namespace std;

enum passenger_type
{
  go,
  pthread
};

struct passenger_data
{
  chrono::high_resolution_clock::time_point start_time, end_time;
  passenger_type type;
  bool boarded;
  //unsigned int boat_no;
};

struct collector 
{
  collector():
    passenger_count(0),
    boat_count(0)
  {}

  unsigned int passenger_count;
  unsigned int boat_count;
  passenger_data passengers[TOTAL_PASSENGER_COUNT];

  mutex mux;
  condition_variable over;

  void lock() { mux.lock(); }
  void unlock() { mux.unlock(); }

  void notify_end()
  {
    mux.lock();
    over.notify_all();
    mux.unlock();
  }

  void wait_for_end()
  {
    unique_lock<mutex> lock(mux);
    while(passenger_count < TOTAL_PASSENGER_COUNT)
      over.wait(lock);
  }
};
