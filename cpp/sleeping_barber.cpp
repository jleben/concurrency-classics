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

struct barber_shop;

struct customer
{
  condition_variable m_signal;
  mutex m_mutex;
  bool m_cutting_done;
  thread m_thread;

  customer( barber_shop * shop ):
    m_cutting_done(false),
    m_thread( &customer::go_cut_hair, this, shop )
  {}

  void cut()
  {
    this_thread::sleep_for(milliseconds(10));
    unique_lock<mutex> locker(m_mutex);
    m_cutting_done = true;
    cout << "Barber: Sir, how do you like it?" << endl;
    m_signal.notify_all();
  }

  void release()
  {
    m_thread.join();
  }

private:
  void wait_cutting_done()
  {
    unique_lock<mutex> locker(m_mutex);
    if (!m_cutting_done)
      m_signal.wait(locker, [&](){ return m_cutting_done; });
  }

  void go_cut_hair( barber_shop *shop );
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

  customer *next_customer()
  {
    unique_lock<mutex> locker(m_mutex);
    if (m_queue.empty())
      return nullptr;

    customer *next_customer = m_queue.front();
    m_queue.pop();
    return next_customer;
  }

  customer *await_next_customer()
  {
    unique_lock<mutex> locker(m_mutex);
    if (m_queue.empty() && m_open)
      m_signal.wait(locker, [&](){ return !m_queue.empty() || !m_open; });

    if (m_queue.empty() && !m_open)
      return nullptr;

    customer *next_customer = m_queue.front();
    m_queue.pop();
    if (m_open)
      cout << "- Queue = " << m_queue.size() << endl;
    else
      cout << "- Queue [after hours] = " << m_queue.size() << endl;
    return next_customer;
  }

  bool take_in(customer *next_customer)
  {
    unique_lock<mutex> locker(m_mutex);
    if (m_queue.size() >= m_queue_limit)
      return false;

    m_queue.push(next_customer);
    cout << "+ Queue = " << m_queue.size() << endl;

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

void customer::go_cut_hair( barber_shop *shop )
{
  bool got_in = shop->take_in(this);
  if (got_in)
  {
    cout << "Customer: Hmmm... how do I want my hair today..." << endl;
    wait_cutting_done();
    cout << "Customer: Just what I wanted!" << endl;
  }
  else
  {
    cout << "Customer: Oh damn. Full again! I'll look like a cow..." << endl;
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

private:
  void work()
  {
    while(true)
    {
      customer *next_customer = m_shop->await_next_customer();
      if (!next_customer)
        break;
      cout << "Barber: Sir, what will it be?" << endl;
      next_customer->cut();
      next_customer->release();
    }
  }
};

int main()
{
  barber_shop shop(3);
  barber otto(&shop);

  minstd_rand r;
  uniform_int_distribution<int> u(2,13);

  int count = 100;
  while(count--)
  {
    new customer(&shop);
    this_thread::sleep_for(milliseconds(u(r)));
  }

  otto.stop();
}


