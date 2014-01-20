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

class agent;

class element
{
protected:
  mutex m_mutex;
  condition_variable m_signal;
  agent *m_agent;
  bool m_bonded;
  thread m_thread;

public:
  element(agent *a):
    m_agent(a),
    m_bonded(false),
    m_thread(&element::bond, this)
  {}

  void signal_bonded()
  {
    m_mutex.lock();
    m_bonded = true;
    m_mutex.unlock();
    m_signal.notify_all();
  }


  void join() { m_thread.join(); }

protected:
  virtual void bond() = 0;
};

class hydrogen;
class oxygen;

class agent
{
  mutex m_mutex;
  condition_variable m_signal;
  queue<hydrogen*> m_h_queue;
  queue<oxygen*> m_o_queue;
  thread m_thread;

public:
  agent():
    m_thread(&agent::run, this)
  {}

  void bond(hydrogen *h)
  {
    unique_lock<mutex> lock(m_mutex);
    m_h_queue.push(h);
    m_signal.notify_all();
  }

  void bond(oxygen *o)
  {
    unique_lock<mutex> lock(m_mutex);
    m_o_queue.push(o);
    m_signal.notify_all();
  }

  void join() { m_thread.join(); }

private:
  void run();
};

class hydrogen : public element
{
public:
  hydrogen(agent * a):
    element(a)
  {}

private:
  void bond()
  {
    cout << "+ H" << endl;
    m_agent->bond(this);
    unique_lock<mutex> lock(m_mutex);
    if (!m_bonded)
      m_signal.wait(lock, [&](){ return m_bonded; });
    cout << "- H" << endl;
  }
};

class oxygen : public element
{
public:
  oxygen(agent * a):
    element(a)
  {}

private:
  void bond()
  {
    cout << "+ O" << endl;
    m_agent->bond(this);
    unique_lock<mutex> lock(m_mutex);
    if (!m_bonded)
      m_signal.wait(lock, [&](){ return m_bonded; });
    cout << "- O" << endl;
  }
};


void agent::run()
{
  auto can_bond = [&]()
  {
    return m_h_queue.size() >= 2 && m_o_queue.size() >= 1;
  };

  unique_lock<mutex> lock(m_mutex);
  while(true)
  {
    if (!can_bond())
      m_signal.wait(lock, can_bond);

    hydrogen *h1 = m_h_queue.front();
    m_h_queue.pop();
    hydrogen *h2 = m_h_queue.front();
    m_h_queue.pop();
    oxygen *o2 = m_o_queue.front();
    m_o_queue.pop();

    h1->signal_bonded();
    h2->signal_bonded();
    o2->signal_bonded();

    h1->join();
    h2->join();
    o2->join();
  }
}

int main()
{
  agent heisenberg;

  minstd_rand r;
  uniform_int_distribution<int> u(0,2);

  minstd_rand r2;
  uniform_int_distribution<int> u2(50,100);
  while(true)
  {
    int choice = u(r);
    if (choice == 2)
      new oxygen(&heisenberg);
    else
      new hydrogen(&heisenberg);

    this_thread::sleep_for(milliseconds(u2(r)));
  }
}
