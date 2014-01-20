#include <mutex>
#include <thread>
#include <chrono>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdint>
#include <atomic>

using namespace std;
using namespace std::chrono;

atomic<bool> g_dine;

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
public:
  philosopher(string name, hand prominent_hand, fork *left_fork, fork * right_fork):
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
      //think();
      //cout << m_name << " Grabbing forks..." << endl;
      grab_forks();
      ++m_count;
      //cout << m_name << " Eating..." << endl;
      //eat();
      //cout << m_name << " Releasing forks..." << endl;
      release_forks();
      //cout << m_name << " Done." << endl;
    }
  }

  void think()
  {
    long ms = (float) std::rand() / RAND_MAX * 10 + 10;
    this_thread::sleep_for( milliseconds(ms) );
  }

  void eat()
  {
    long ms = (float) std::rand() / RAND_MAX * 10 + 10;
    this_thread::sleep_for( milliseconds(ms) );
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

  constexpr int count = 30;

  fork forks[count];

  philosopher *diners[count];

  for (int idx = 0; idx < count; ++idx)
  {
    ostringstream name;
    name << idx;
    diners[idx] = new philosopher(name.str(),
                                  (idx == 0) ? left_hand : right_hand,
                                  //(idx % 2 == 0 ? left_hand : right_hand),
                                  &forks[idx], &forks[(idx+1) % count]);
  }

  this_thread::sleep_for(seconds(1));

  g_dine = false;

  for (int idx = 0; idx < count; ++idx)
  {
    diners[idx]->join();
    cout << idx << ": " << diners[idx]->eaten_count() << endl;
  }
}
