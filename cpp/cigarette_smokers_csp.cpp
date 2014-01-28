#include "csp.hpp"
#include "util.hpp"

#include <cstdio>
#include <chrono>
#include <array>
#include <random>

using namespace std;
using namespace std::chrono;

constexpr int g_max_smoke_count(10000);
atomic<int> g_total_smoke_count(0);
int g_smoke_count[3] = {0,0,0};

high_resolution_clock::time_point g_start_time;
high_resolution_clock::time_point g_end_time;

bool g_done = false;
condition_variable g_done_signal;
mutex g_mutex;

void wait_done()
{
  unique_lock<mutex> lock(g_mutex);
  while(!g_done)
    g_done_signal.wait(lock);
}

void notify_done()
{
  unique_lock<mutex> lock(g_mutex);
  g_done = true;
  g_done_signal.notify_all();
}

class smoker : public process
{
  signal m_ingredients;
  int m_index;

public:
  smoker(int idx): m_index(idx) {}
  void give_ingredients() { notify(m_ingredients); }

private:
  void work()
  {
    dummy_worker dummy(1e4);

    while(true)
    {
      //PRINT("Smoker " << m_index << " waiting.");

      await(m_ingredients);

      PRINT("Smoker " << m_index << " smoking.");

      dummy.work();

      int total_smoke_count = ++g_total_smoke_count;

      if (total_smoke_count == 1)
        g_start_time = high_resolution_clock::now();

      if (total_smoke_count == g_max_smoke_count)
      {
        g_end_time = high_resolution_clock::now();
        notify_done();
      }

      if (total_smoke_count >= g_max_smoke_count)
        break;

      ++g_smoke_count[m_index];
    }
  }
};

class dealer : public process
{
  array<smoker*,3> smokers;
public:
  dealer(smoker *s1, smoker *s2, smoker *s3)
  {
    smokers[0] = s1;
    smokers[1] = s2;
    smokers[2] = s3;
  }
private:
  void work()
  {
    random_device rd;
    minstd_rand R(rd());
    uniform_int_distribution<int> U(0,2);

    while(true)
    {
      int i = U(R);
      smokers[i]->give_ingredients();
    }
  }
};

int main()
{
  smoker s1(0);
  smoker s2(1);
  smoker s3(2);
  dealer d { &s1, &s2, &s3 };

  s1.go();
  s2.go();
  s3.go();
  d.go();

  wait_done();

  for(int i = 0; i < 3; ++i)
  {
    cout << "Smoker " << i << " has smoked " << g_smoke_count[i] << endl;
  }

  duration<double,milli> total_time = g_end_time - g_start_time;
  cout << "Total time = " << total_time.count() << endl;
}
