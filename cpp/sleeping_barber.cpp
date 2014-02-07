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

using namespace std;
using namespace std::chrono;

#define MEASURE 1

atomic<int> g_done_customer_count(0);
constexpr int g_total_customer_count(10000);

struct barber_shop;

struct customer
{
  condition_variable m_signal;
  mutex m_mutex;
  bool m_cutting_done;
  thread m_thread;

  customer( barber_shop * shop ):
    m_cutting_done(false),
    m_thread( &customer::run, this, shop )
  {}

  void notify_done()
  {
    unique_lock<mutex> locker(m_mutex);
    m_cutting_done = true;
    m_signal.notify_all();
  }

private:
  void wait_cutting_done()
  {
    unique_lock<mutex> locker(m_mutex);
    while(!m_cutting_done)
      m_signal.wait(locker);
  }

  void run( barber_shop *shop );
};

struct barber_shop
{
  queue<customer*> m_queue;
  int m_queue_limit;
  mutex m_mutex;
  condition_variable m_signal;
  bool m_open;

  barber_shop(int queue_limit):
    m_queue_limit(queue_limit),
    m_open(true)
  {

  }

  customer *await_next_customer()
  {
    unique_lock<mutex> locker(m_mutex);
    while(m_queue.empty() && m_open)
      m_signal.wait(locker);

    if (m_queue.empty())
      return nullptr;

    customer *next_customer = m_queue.front();
    m_queue.pop();

    if (m_open)
    {
      PRINT("- Queue = " << m_queue.size());
    }
    else
    {
      PRINT("- Queue [after hours] = " << m_queue.size());
    }
    return next_customer;
  }

  bool take_in(customer *next_customer)
  {
    unique_lock<mutex> locker(m_mutex);

#if !(MEASURE)
    if (m_queue.size() >= m_queue_limit)
      return false;
#endif

    m_queue.push(next_customer);
    PRINT("+ Queue = " << m_queue.size());

    m_signal.notify_all();
    return true;
  }

  void close()
  {
    unique_lock<mutex> locker(m_mutex);
    m_open = false;
    m_signal.notify_all();
  }
};

void customer::run( barber_shop *shop )
{
#if !(MEASURE)
  random_device rd;
  minstd_rand R(rd());
  uniform_int_distribution<int> U(200,600);
#endif

  while(true)
  {
    m_cutting_done = false;

    bool got_in = shop->take_in(this);
    if (got_in)
    {
      PRINT("Customer: Waiting is boooooring...");
      wait_cutting_done();
      PRINT("Customer: Thanks for the shave!");
    }
    else
    {
      PRINT("Customer: Oh damn. Full again! I'll look like a bear...");
    }
#if !(MEASURE)
    this_thread::sleep_for(microseconds(U(R)));
#endif
  }
}

struct barber
{
  barber_shop *m_shop;
  thread m_thread;

  barber( barber_shop *shop ):
    m_shop(shop),
    m_thread(&barber::work, this)
  {}

  void stop()
  {
    m_shop->close();
    m_thread.join();
  }

  void join()
  {
    m_thread.join();
  }

private:
  void work()
  {
    dummy_worker dummy(1e4);
    while(g_done_customer_count < g_total_customer_count)
    {
      PRINT("Barber: waiting...");
      customer *next_customer = m_shop->await_next_customer();
      if (!next_customer)
        break;
      PRINT("Barber: got customer.");
      dummy.work();
      PRINT("Barber: done customer.");
      next_customer->notify_done();
      ++g_done_customer_count;
    }
  }
};

int main()
{
  barber_shop shop(15);
  barber otto(&shop);

  high_resolution_clock::time_point start_time = high_resolution_clock::now();

  int count = 20;
  while(count--)
  {
    new customer(&shop);
  }

  otto.join();

  high_resolution_clock::time_point end_time = high_resolution_clock::now();

  //this_thread::sleep_for(milliseconds(1000));
  //int done_count = g_done_customer_count;
  //cout << "Done: " << done_count << endl;

  duration<double,milli> total_time = end_time - start_time;
  cout << "Total time = " << total_time.count() << endl;
}


