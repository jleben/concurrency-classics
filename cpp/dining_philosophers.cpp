#include "util.hpp"

#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdint>
#include <atomic>
#include <random>

using namespace std;
using namespace std::chrono;

constexpr int g_philosoper_count = 5;
constexpr int g_max_eatings_count(10000);

atomic<bool> g_dine;
atomic<int> g_eatings_count(0);

random_device rd;
minstd_rand R(rd());

high_resolution_clock::time_point g_start_time;
high_resolution_clock::time_point g_end_time;

enum hand
{
  left_hand,
  right_hand
};

struct fork
{
  void grab()
  {
    m_mutex.lock();
  }

  void release()
  {
    m_mutex.unlock();
  }

private:
  mutex m_mutex;
};

class philosopher
{
  dummy_worker m_worker;
public:
  philosopher(string name, hand prominent_hand, fork *left_fork, fork * right_fork):
    m_worker(1e4),
    m_name(name),
    m_prominent_hand(prominent_hand),
    m_left_fork(left_fork),
    m_right_fork(right_fork),
    m_count(0),
    m_thread(&philosopher::dine, this)
  {

  }

  void join() { m_thread.join(); }

  uint64_t eaten_count() { return m_count; }

private:

  void dine()
  {
    while(g_dine)
    {
      //cout << m_name << " Thinking..." << endl;
      think();
      //cout << m_name << " Grabbing forks..." << endl;
      grab_forks();
      //cout << m_name << " Eating..." << endl;
      eat();
      //cout << m_name << " Releasing forks..." << endl;
      release_forks();
      //cout << m_name << " Done." << endl;

      int all_eatings_count = ++g_eatings_count;
      if (all_eatings_count == 1)
        g_start_time = high_resolution_clock::now();
      if (all_eatings_count == g_max_eatings_count)
        g_end_time = high_resolution_clock::now();
      if (all_eatings_count <= g_max_eatings_count)
        ++m_count;
      if (all_eatings_count >= g_max_eatings_count)
        break;
    }
  }

  void think()
  {
    m_worker.work();
    //long ms = (float) std::rand() / RAND_MAX * 10 + 10;
    //this_thread::sleep_for( milliseconds(ms) );
  }

  void eat()
  {
    m_worker.work();

    //long ms = (float) std::rand() / RAND_MAX * 10 + 10;
    //this_thread::sleep_for( milliseconds(ms) );
  }

  void grab_forks()
  {
    if (m_prominent_hand == left_hand)
    {
      m_left_fork->grab();
      m_right_fork->grab();
    }
    else
    {
      m_right_fork->grab();
      m_left_fork->grab();
    }
  }

  void release_forks()
  {
    if (m_prominent_hand == left_hand)
    {
      m_right_fork->release();
      m_left_fork->release();
    }
    else
    {
      m_left_fork->release();
      m_right_fork->release();
    }
  }

private:
  string m_name;
  const hand m_prominent_hand;
  fork *m_left_fork;
  fork *m_right_fork;
  uint64_t m_count;
  thread m_thread;
};

int main()
{
  g_dine = true;

  constexpr int count = g_philosoper_count;

  fork forks[count];

  philosopher *diners[count];

  for (int idx = 0; idx < count; ++idx)
  {
    ostringstream name;
    name << idx;
    diners[idx] = new philosopher(name.str(),
                                  (idx % 2 == 0 ? left_hand : right_hand),
                                  &forks[idx], &forks[(idx+1) % count]);
  }

  //this_thread::sleep_for(seconds(1));

  //g_dine = false;

  for (int idx = 0; idx < count; ++idx)
  {
    diners[idx]->join();
    cout << idx << ": " << diners[idx]->eaten_count() << endl;
  }

  duration<double, milli> total_duration = g_end_time - g_start_time;
  cout << "Total time = " << total_duration.count() << endl;
}
