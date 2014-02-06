#include "util.hpp"

#include <mutex>
#include <condition_variable>
#include <thread>
#include <random>
#include <atomic>

using namespace std;
using namespace std::chrono;

constexpr int g_max_smoke_count(10000);
atomic<int> g_total_smoke_count(0);
int g_smoke_count[3] = {0,0,0};

high_resolution_clock::time_point g_start_time;
high_resolution_clock::time_point g_end_time;

bool done = false;
condition_variable g_done;

class table
{
  std::mutex m_mux;
  std::condition_variable m_condition;
  int m_smoker_index;

  bool m_done;
  condition_variable m_done_signal;

public:
  table():
    m_smoker_index(-1),
    m_done(false)
  {}

  void agent()
  {
    random_device rd;
    minstd_rand R(rd());
    uniform_int_distribution<int> U(0, 2);

    while(true)
    {
      int next_smoker_index = U(R);

      unique_lock<mutex> lock(m_mux);

      while (m_smoker_index != -1)
        m_condition.wait(lock);

      m_smoker_index = next_smoker_index;
      m_condition.notify_all();
    }
  }

  void smoker( int index )
  {
    dummy_worker dummy(1e4);

    while(true)
    {
      {
        unique_lock<mutex> lock(m_mux);

        while(m_smoker_index != index)
          m_condition.wait(lock);

        m_smoker_index = -1;
        m_condition.notify_all();
      }

      PRINT("Smoker " << index << " smoking.");
      dummy.work();

      int total_smoke_count = ++g_total_smoke_count;

      if (total_smoke_count == 1)
        g_start_time = high_resolution_clock::now();

      if (total_smoke_count == g_max_smoke_count)
      {
        g_end_time = high_resolution_clock::now();
        unique_lock<mutex> lock(m_mux);
        m_done = true;
        m_done_signal.notify_all();
      }

      if (total_smoke_count >= g_max_smoke_count)
        break;

      ++g_smoke_count[index];
    }
  }

  void wait_done()
  {
    unique_lock<mutex> lock(m_mux);
    while(!m_done)
      m_done_signal.wait(lock);
  }
};

int main()
{
  table t;
  thread a(&table::agent, &t);
  thread s[3];
  for(int i = 0; i < 3; ++i)
    s[i] = thread(&table::smoker, &t, i);

  t.wait_done();

  for(int i = 0; i < 3; ++i)
  {
    cout << "Smoker " << i << " has smoked " << g_smoke_count[i] << endl;
  }

  duration<double,milli> total_time = g_end_time - g_start_time;
  cout << "Total time = " << total_time.count() << endl;
}
