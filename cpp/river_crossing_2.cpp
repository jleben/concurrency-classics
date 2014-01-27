#include "river_crossing.hpp"

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
#include <algorithm>

using namespace std;
using namespace std::chrono;

collector metrics;

struct boat
{
public:

  boat():
    waiting_goers(0),
    waiting_pthreaders(0),
    allowed_goers(0),
    allowed_pthreaders(0),
    boarding(false)
  {}

  void run()
  {
    random_device rd;
    static minstd_rand R(rd());
    static uniform_int_distribution<int> U(0,1);

    while(true)
    {

      metrics.lock();
      int passenger_index = metrics.arrived_passenger_count++;
      int passenger_count = metrics.arrived_passenger_count;
      metrics.unlock();

      if (passenger_count >= TOTAL_PASSENGER_COUNT)
        break;

      int passenger_type_coin = U(R);
      passenger_type p = passenger_type_coin < 1 ? go : pthread;

      metrics.passengers[passenger_index].type = p;
      metrics.passengers[passenger_index].start_time = high_resolution_clock::now();

      embark(p);

      metrics.passengers[passenger_index].end_time = high_resolution_clock::now();
      metrics.passengers[passenger_index].boarded = true;

      this_thread::sleep_for(milliseconds(THREAD_SLEEP_TIME));
    }

    metrics.notify_end();
  }

private:
  void embark( passenger_type p )
  {
    unique_lock<mutex> lock(m_mutex);

    if (p == go)
      ++waiting_goers;
    else
      ++waiting_pthreaders;

    if (p == go)
    {
      while(!allowed_goers)
      {
        if (boarding || !try_form_group(p))
          m_go_can_board.wait(lock);
      }

      --waiting_goers;
      --allowed_goers;
    }
    else
    {
      while(!allowed_pthreaders)
      {
        if (boarding || !try_form_group(p))
          m_pthread_can_board.wait(lock);
      }

      --waiting_pthreaders;
      --allowed_pthreaders;
    }

    //board(p);

    if (allowed_goers == 0 && allowed_pthreaders == 0)
    {
      boarding = false;
      m_go_can_board.notify_one();
      //row();

    }
  }

  void board(passenger_type p)
  {
    //group += p == go ? 'G' : 'P';
  }

  void row()
  {
    /*
    cout << group;

    if (group.size() != 4 ||
        count(group.begin(), group.end(), 'G') == 1 ||
        count(group.begin(), group.end(), 'P') == 1)
    {
      cout << " ERROR!";
    }

    cout << endl;

    group.clear();
    */
  }

  bool try_form_group(passenger_type p)
  {
    if (boarding)
      return false;

    if (waiting_goers >= 2 && waiting_pthreaders >= 2)
    {
      allowed_goers = 2;
      allowed_pthreaders = 2;
      if (p == go)
      {
        notify_go(1);
        notify_pthread(2);
      }
      else
      {
        notify_go(2);
        notify_pthread(1);
      }
    }
    else if (waiting_goers >= 4)
    {
      allowed_goers = 4;
      if (p == go)
      {
        notify_go(3);
      }
      else
      {
        notify_go(4);
      }
    }
    else if (waiting_pthreaders >= 4)
    {
      allowed_pthreaders = 4;
      if (p == go)
      {
        notify_pthread(4);
      }
      else
      {
        notify_pthread(3);
      }
    }
    else
      return false;

#if 0
    cout << "waiting_goers = " << waiting_goers << endl;
    cout << "waiting_pthreaders = " << waiting_pthreaders << endl;
    cout << "allowed_goers = " << allowed_goers << endl;
    cout << "allowed_pthreaders = " << allowed_pthreaders << endl;
#endif
    boarding = true;
    return true;
  }

  void notify_go(int count)
  {
    while(count--)
      m_go_can_board.notify_one();
  }

  void notify_pthread(int count)
  {
    while(count--)
      m_pthread_can_board.notify_one();
  }

  mutex m_mutex;
  condition_variable m_go_can_board;
  condition_variable m_pthread_can_board;

  int waiting_goers;
  int waiting_pthreaders;

  int allowed_goers;
  int allowed_pthreaders;

  bool boarding;

  string group;
};

int main()
{
  boat b;

  const int thread_count = 10;
  thread t[thread_count];

  for (int i = 0; i < thread_count; ++i)
  {
    t[i] = thread(&boat::run, &b);
  }

  metrics.wait_for_end();

  cout << "over" << endl;

  high_resolution_clock::duration avg_time(0);
  size_t boarded_count = 0;

  for (int i = 0; i < TOTAL_PASSENGER_COUNT; ++i)
  {
    passenger_data &p = metrics.passengers[i];
    cout << "Passenger " << i << ":";
    if (p.boarded)
    {
      ++boarded_count;
      auto time = p.end_time - p.start_time;
      avg_time += time;
      duration<double, micro> print_time = time;
      cout << " waiting time:" << print_time.count();
    }
    else
    {
      cout << " not boarded";
    }
    cout << endl;
  }

  avg_time /= boarded_count;
  cout << " avg time = " << duration<double, micro>(avg_time).count() << endl;

#if 0
  for (int i = 0; i < thread_count; ++i)
    t[i].join();
#endif
}
