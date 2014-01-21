#define BERKELEY_SOLUTION 0
#define UVIC_SOLUTION 1

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

struct element
{
  bool bonded;
  element():
    bonded(false)
  {}
};

struct hydrogen : public element
{

};

struct oxygen : public element
{

};

  // The official Berkeley solution does not ensure threads actually run
  // in correct order (as opposed to become runnable in correct order).
  // Consider the case where there is an abundance of H available, and two
  // O come tightly one after another. The first O will succeed to acquire
  // the mutex, and the second one will attempt to acquire it before any
  // H is awaken. When an H is awaken, it needs to acquire the mutex, but the
  // seconds O will get it before, continuing straight to "bond()" because
  // there's plenty of H available.
  // Thus, two Os will call "bond()" in succession.

#if BERKELEY_SOLUTION
class barrier
{
  int available_h;
  int available_o;
  int bonded_h;
  int bonded_o;

  condition_variable m_o_condition;
  condition_variable m_h_condition;

  string m_molecule;

  mutex m_mutex;

  void bond(char element)
  {
    m_molecule += element;

    if (m_molecule.size() == 3)
    {
      cout << m_molecule;
      if (std::count(m_molecule.begin(), m_molecule.end(), 'H') != 2)
        cout << " ERROR!";
      cout << endl;

      m_molecule.clear();
    }
  }

public:
  barrier():
    available_h(0),
    available_o(0),
    bonded_h(0),
    bonded_o(0)
  {}

  void process_hydrogen()
  {
    unique_lock<mutex> lock(m_mutex);

    available_h++;

    // while not allowed to leave
    while (bonded_h == 0) {
      // try to make a water molecule
      if (available_h >= 2 && available_o >= 1) {
        available_h-=2; bonded_h+=2;
        available_o-=1; bonded_o+=1;
        m_h_condition.notify_one();
        m_o_condition.notify_one();
      }
      // else wait for somebody else to
      else {
        m_h_condition.wait(lock);
      }
    }
    bonded_h--;
    bond('H');
  }

  void process_oxygen()
  {
    unique_lock<mutex> lock(m_mutex);

    available_o++;

    // while not allowed to leave
    while (bonded_o == 0) {
      // try to make a water molecule
      if (available_h >= 2 && available_o >= 1) {
        available_h-=2; bonded_h+=2;
        available_o-=1; bonded_o+=1;
        m_h_condition.notify_one();
        m_h_condition.notify_one();
      }
      // else wait for somebody else to
      else {
        m_o_condition.wait(lock);
      }
    }
    bonded_o--;
    bond('O');
  }
};
#endif


#if UVIC_SOLUTION

struct barrier
{
  mutex m_mutex;

  condition_variable m_stage_gate;
  condition_variable m_bond_gate;

  int staged_h;
  int staged_o;

  uint64_t queued_h;
  uint64_t queued_o;

  string m_molecule;

public:

  barrier():
    staged_h(0),
    staged_o(0),
    queued_h(0),
    queued_o(0)
  {}

  bool stage_complete()
  {
    return staged_h == 2 && staged_o == 1;
  }

  bool molecule_complete()
  {
    return m_molecule.size() == 3;
  }

  void release_molecule()
  {
    cout << m_molecule;
    if (std::count(m_molecule.begin(), m_molecule.end(), 'H') != 2)
      cout << " ERROR!";
    cout << endl;

    m_molecule.clear();
  }

  void process_hydrogen()
  {
    bool queued = false;

    unique_lock<mutex> lock(m_mutex);

    while (staged_h == 2)
    {
      if (!queued)
      {
        ++queued_h;
        //cout << "queued H = " << queued_h << endl;
      }
      queued = true;
      m_stage_gate.wait(lock);
    }

    if (queued)
      --queued_h;

    ++staged_h;
    //cout << "staged H" << endl;

    while (!stage_complete())
      m_bond_gate.wait(lock);

    m_bond_gate.notify_all();

    m_molecule += 'H';
    //cout << "bonded H" << endl;

    if (molecule_complete())
    {
      release_molecule();
      staged_h = 0;
      staged_o = 0;
      m_stage_gate.notify_all();
    }
  }

  void process_oxygen()
  {
    bool queued = false;

    unique_lock<mutex> lock(m_mutex);

    while (staged_o == 1)
    {
      if (!queued)
      {
        ++queued_o;
        //cout << "queued O = " << queued_o << endl;
      }
      queued = true;
      m_stage_gate.wait(lock);
    }

    if (queued)
      --queued_o;

    ++staged_o;
    //cout << "staged O" << endl;

    while (!stage_complete())
      m_bond_gate.wait(lock);

    m_bond_gate.notify_all();

    m_molecule += 'O';
    //cout << "bonded O" << endl;

    if (molecule_complete())
    {
      release_molecule();
      staged_h = 0;
      staged_o = 0;
      m_stage_gate.notify_all();
    }
  }
};
#endif

int main()
{
  barrier b;

  minstd_rand r;
  uniform_int_distribution<int> u(0,2);

  minstd_rand r2;
  uniform_int_distribution<int> u2(10, 30);

  int count = 10000;

  while(true)
  {
    cout << "------------------" << endl;

    int batch_count = 50;
    while( batch_count-- )
    {
      int choice = u(r);
      if (choice == 2)
      {
        thread t( &barrier::process_oxygen, &b );
        t.detach();
      }
      else
      {
        thread t( &barrier::process_hydrogen, &b );
        t.detach();
      }
    }
    //this_thread::sleep_for(milliseconds(u2(r)));
    this_thread::sleep_for(milliseconds(500));
  }

  cout << "ending..." << endl;
  this_thread::sleep_for(milliseconds(100));
}
