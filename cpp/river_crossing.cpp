
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

enum programmer
{
  goer,
  pthreader
};

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

  void enqueue( programmer p )
  {
    unique_lock<mutex> lock(m_mutex);

    if (p == goer)
      ++waiting_goers;
    else
      ++waiting_pthreaders;

    if (p == goer)
    {
      while(!allowed_goers)
      {
        if (boarding || !try_form_group())
          m_board_gate.wait(lock);
      }

      --waiting_goers;
      --allowed_goers;
    }
    else
    {
      while(!allowed_pthreaders)
      {
        if (boarding || !try_form_group())
          m_board_gate.wait(lock);
      }

      --waiting_pthreaders;
      --allowed_pthreaders;
    }

    board(p);

    if (allowed_goers == 0 && allowed_pthreaders == 0)
    {
      boarding = false;
      m_board_gate.notify_all();
      row();
    }
  }

  void board(programmer p)
  {
    group += p == goer ? 'G' : 'P';
  }

  void row()
  {
    cout << group;

    if (group.size() != 4 ||
        count(group.begin(), group.end(), 'G') == 1 ||
        count(group.begin(), group.end(), 'P') == 1)
    {
      cout << " ERROR!";
    }

    cout << endl;

    group.clear();
  }

private:
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

  minstd_rand r;
  uniform_int_distribution<int> u(0,4);

  minstd_rand r2;
  uniform_int_distribution<int> u2(10, 30);

  while(true)
  {
    cout << "------------------" << endl;

    int batch_count = 50;
    while( batch_count-- )
    {
      int choice = u(r);
      programmer p = choice < 4 ? goer : pthreader;

      thread t( &boat::enqueue, &b, p );
      t.detach();
    }
    //this_thread::sleep_for(milliseconds(u2(r)));
    this_thread::sleep_for(milliseconds(500));
  }

  cout << "ending..." << endl;
  this_thread::sleep_for(milliseconds(100));
}
