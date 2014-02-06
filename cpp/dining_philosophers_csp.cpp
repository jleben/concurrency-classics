#include "util.hpp"
#include "csp.hpp"

#include <cstdio>
#include <chrono>
#include <array>

using namespace std;
using namespace std::chrono;

constexpr int g_philosoper_count = 5;
constexpr int g_max_eatings_count(10000);

atomic<bool> g_dine(true);
atomic<int> g_eatings_count(0);

class fork : public process
{
  signal m_left_picked;
  signal m_left_dropped;
  signal m_right_picked;
  signal m_right_dropped;
  const bool m_start_left;
public:
  fork(bool start_left):
    m_start_left(start_left)
  {}

  void left_pick() { notify(m_left_picked); }
  void left_drop() { notify(m_left_dropped); }
  void right_pick() { notify(m_right_picked); }
  void right_drop() { notify(m_right_dropped); }

private:
  void work()
  {
    if (m_start_left)
    {
      await(m_left_picked);
      await(m_left_dropped);
    }
    while (true)
    {
      await(m_right_picked);
      await(m_right_dropped);
      await(m_left_picked);
      await(m_left_dropped);
    }
  }
};

class philosopher : public process
{
  const int m_index;
  fork *m_left_fork;
  fork *m_right_fork;
  unsigned int m_bite_count;

public:
  philosopher(int index, fork *left_fork, fork *right_fork):
    m_index(index),
    m_left_fork(left_fork),
    m_right_fork(right_fork),
    m_bite_count(0)
  {}
  int index() const { return m_index; }
  unsigned int bite_count() const { return m_bite_count; }

private:
  void work()
  {
    while(g_dine)
    {
      m_left_fork->right_pick();
      m_right_fork->left_pick();
      PRINT(m_index <<  " eating");
      m_bite_count++;
      m_left_fork->right_drop();
      m_right_fork->left_drop();
    }
  }
};

int main()
{
  static constexpr int count = 5;
  philosopher *posse[count];
  fork *forks[count];

  {
    int idx = count;
    while(idx--)
    {
      bool start_left = (idx % 2) == 0;
      forks[idx] = new fork(start_left);
      forks[idx]->go();
    }
  }

  {
    int idx = count;
    while(idx--)
    {
      posse[idx] = new philosopher(idx, forks[idx], forks[(idx + 1) % count]);
      posse[idx]->go();
    }
  }

  this_thread::sleep_for(milliseconds(1000));
  g_dine = false;

  {
    for(philosopher *p : posse)
    {
      p->join();
    }

    for(philosopher *p : posse)
    {
      cout << p->index() << " = " << p->bite_count() << endl;
    }
  }


}
