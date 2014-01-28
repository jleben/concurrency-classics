#include "river_crossing.hpp"
#include "util.hpp"

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
#include <cstdint>

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
      this_thread::sleep_for(milliseconds(THREAD_SLEEP_TIME));

      int passenger_index = metrics.arrived_passenger_count++;

      int passenger_type_coin = U(R);
      passenger_type p = passenger_type_coin < 1 ? go : pthread;

      high_resolution_clock::time_point start_time = high_resolution_clock::now();

      bool is_captain;
      embark(p, is_captain);

      high_resolution_clock::time_point end_time = high_resolution_clock::now();

      if (is_captain)
      {
        int boat_count = ++metrics.boat_count;
        if (boat_count == max_boat_count)
            metrics.notify_end();
      }

      int departed_passenger_index = metrics.departed_passenger_count++;
      if (departed_passenger_index >= max_passenger_count)
        break;

      passenger_data & metric = metrics.passengers[departed_passenger_index];
      metric.type = p;
      metric.boarded = true;
      metric.start_time = start_time;
      metric.end_time = end_time;
    }
  }

private:
  void embark( passenger_type p, bool & is_captain )
  {
    unique_lock<mutex> lock(m_mutex);

    int & waiting_count = p == go ? waiting_goers : waiting_pthreaders;
    int & allowed_count = p == go ? allowed_goers : allowed_pthreaders;

    ++waiting_count;

    while(!allowed_count)
    {
      if (!try_form_group())
        m_board_gate.wait(lock);
    }

    --waiting_count;
    --allowed_count;

    board(p);

    if (allowed_goers == 0 && allowed_pthreaders == 0)
    {
      is_captain = true;
      boarding = false;
      m_board_gate.notify_all();
      row();
    }
    else
    {
      is_captain = false;
    }
  }

  void board(passenger_type p)
  {
    group += p == go ? 'G' : 'P';
  }

  void row()
  {
    PRINT(group);
    /*
    if (group.size() != 4 ||
        count(group.begin(), group.end(), 'G') == 1 ||
        count(group.begin(), group.end(), 'P') == 1)
    {
      cout << " ERROR!";
    }
    */
    group.clear();
  }

  bool try_form_group()
  {
    if (boarding)
      return false;

    if (waiting_goers >= 2 && waiting_pthreaders >= 2)
    {
      allowed_goers = 2;
      allowed_pthreaders = 2;
    }
    else if (waiting_goers >= 4)
    {
      allowed_goers = 4;
    }
    else if (waiting_pthreaders >= 4)
    {
      allowed_pthreaders = 4;
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
    m_board_gate.notify_all();
    return true;
  }

  mutex m_mutex;
  condition_variable m_board_gate;

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

  const int thread_count = THREAD_COUNT;
  thread t[thread_count];

  high_resolution_clock::time_point external_start_time = high_resolution_clock::now();

  for (int i = 0; i < thread_count; ++i)
  {
    t[i] = thread(&boat::run, &b);
  }

  metrics.wait_for_end();

  high_resolution_clock::time_point external_end_time = high_resolution_clock::now();

  cout << "over" << endl;

  high_resolution_clock::duration avg_time(0);
  high_resolution_clock::duration max_time(0);
  uint64_t time_std = 0;

  size_t boarded_count = 0;

  for (int i = 0; i < max_passenger_count; ++i)
  {
    passenger_data &p = metrics.passengers[i];
    cout << "Passenger " << i << ":";
    if (p.boarded)
    {
      ++boarded_count;
      auto time = p.end_time - p.start_time;
      avg_time += time;
      if (time > max_time)
        max_time = time;

      duration<double, micro> print_time = time;
      cout << " Time = " << print_time.count();
    }
    else
    {
      cout << " not boarded";
    }
    cout << endl;
  }

  avg_time /= boarded_count;

  for (int i = 0; i < max_passenger_count; ++i)
  {
    passenger_data &p = metrics.passengers[i];
    if (p.boarded)
    {
      auto time = p.end_time - p.start_time;
      int64_t t = time.count();
      int64_t avg = avg_time.count();
      int64_t d = t - avg;
      time_std += d * d;
    }
  }

  time_std /= boarded_count;
  time_std = std::sqrt(time_std);

  high_resolution_clock::duration total_time =
      metrics.passengers[max_passenger_count-1].end_time -
      metrics.passengers[0].end_time;

  duration<double, milli> external_total_time =
      external_end_time - external_start_time;

  cout << "Total time = " << duration<double, milli>(total_time).count() << " ms " << endl;
  cout << "Total time (external) = " << external_total_time.count() << " ms " << endl;
  cout << "Average time = " << duration<double, micro>(avg_time).count() << " us "<< endl;
  cout << "Max time = " << duration<double, micro>(max_time).count() << " us " << endl;
  cout << "Time STD = " << duration<double, micro>( high_resolution_clock::duration(time_std) ).count() << " us " << endl;

#if 0
  for (int i = 0; i < thread_count; ++i)
    t[i].join();
#endif
}
