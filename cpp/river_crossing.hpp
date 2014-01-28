/* Header file for River Crossing Program */
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

#define THREAD_COUNT 50
#define THREAD_SLEEP_TIME 0

constexpr int max_boat_count = 1000;
constexpr int max_passenger_count = max_boat_count * 4;

using namespace std;

enum passenger_type
{
  go,
  pthread
};

struct passenger_data
{
  passenger_data():
    boarded(false)
  {}

  std::chrono::high_resolution_clock::time_point start_time, end_time;
  passenger_type type;
  atomic<bool> boarded;
  //unsigned int boat_no;
};

struct collector 
{
  collector():
    arrived_passenger_count(0),
    departed_passenger_count(0),
    boat_count(0)
  {}

  atomic<int> arrived_passenger_count;
  atomic<int> departed_passenger_count;
  atomic<int> boat_count;

  passenger_data passengers[max_passenger_count];

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
    while(boat_count < max_boat_count)
      over.wait(lock);
  }
};
