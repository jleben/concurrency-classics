
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
#include <functional>


using namespace std;
using namespace std::chrono;


class process
{
  mutex m_mutex;
  unique_lock<mutex> m_private_lock;
  condition_variable m_alarm;
  thread m_thread;
  std::vector<bool> m_signals;
  bool m_run;

public:
  process(unsigned int signal_count = 1):
    m_private_lock(m_mutex, defer_lock),
    m_signals(signal_count, false),
    m_run(false)
  {}

  void go()
  {
    m_run = true;
    m_thread = thread(&process::run, this);
  }

  void stop()
  {
    unique_lock<mutex> public_lock(m_mutex);
    m_run = false;
    m_alarm.notify_one();
  }

  void signal(int sig = 0)
  {
    unique_lock<mutex> public_lock(m_mutex);
    m_signals[sig] = true;
    m_alarm.notify_one();
  }

  void join()
  {
    m_thread.join();
  }

protected:
  virtual void work() = 0;

  bool await(int signal = 0)
  {
    while(m_run && !m_signals[signal])
    {
      m_alarm.wait(m_private_lock);
    }
    if (m_run)
      m_signals[signal] = false;

    return m_run;
  }

  int await_any()
  {
    int signalled = -1;

    auto get_signalled = [&]()
    {
      for (int i = 0; i < m_signals.size(); ++i)
      {
        if (m_signals[i])
        {
          signalled = i;
          return true;
        }
      }
      return false;
    };

    while(m_run && !get_signalled())
    {
      m_alarm.wait(m_private_lock);
    }

    if (!m_run)
      signalled = -1;

    if (signalled != -1)
      m_signals[signalled] = false;

    return signalled;
  }

  template< class Rep, class Period >
  bool sleep_for( const std::chrono::duration<Rep,Period>& sleep_duration )
  {
    m_private_lock.unlock();
    this_thread::sleep_for(sleep_duration);
    m_private_lock.lock();
    return m_run;
  }

private:
  virtual void run()
  {
    m_private_lock.lock();
    work();
    m_private_lock.unlock();
  }
};

class elf;
class elk;
class santa_claus;

struct santas_place
{
  mutex mux;

public:
  queue<elf*> queued_elves;
  vector<elf*> elves_at_santa;
  vector<elk*> available_elks;
  int elves_in_service_count;

  santas_place():
    elves_in_service_count(false)
  {}

  void lock()
  {
    mux.lock();
  }

  void unlock()
  {
    mux.unlock();
  }
};

class elf : public process
{
public:
  int id;
  santas_place *place;
  santa_claus *santa;
  elf( int id, santas_place *p, santa_claus *s ):
    id(id),
    place(p),
    santa(s)
  {}

private:
  void work();
};

class elk : public process
{
public:
  int id;
  santas_place *place;
  santa_claus *santa;
  elk( int id, santas_place *p, santa_claus *s ):
    id(id),
    place(p),
    santa(s)
  {}

private:
  void work();
};

class santa_claus : public process
{
public:
  enum signal_type
  {
    elves_need_help = 0,
    elf_finished_consulting,
    elks_ready,
    signal_count
  };

  santas_place *place;
  santa_claus *santa;
  santa_claus( santas_place *p ):
    process(signal_count),
    place(p),
    santa()
  {}

private:
  void work();
};


void elf::work()
{
  while(true)
  {
    if (!sleep_for(milliseconds(80)))
      return;

    ///////////////////

    bool wake_up_santa = false;

    place->lock();
    cout << "Elf " << id << ": waiting." << endl;
    place->queued_elves.push(this);
    if (place->queued_elves.size() >= 3 && !place->elves_in_service_count)
    {
      place->elves_in_service_count = 3;
      wake_up_santa = true;
      cout << "Elf " << id << ": will wake up santa." << endl;
    }
    place->unlock();

    if (wake_up_santa)
      santa->signal(santa_claus::elves_need_help);

    if (await() == -1)
      return;

    place->lock();
    cout << "Elf " << id << ": got help." << endl;
    --place->elves_in_service_count;
    place->unlock();

    santa->signal(santa_claus::elf_finished_consulting);
  }
}

void elk::work()
{
  while(true)
  {
    if (!sleep_for(milliseconds(300)))
      return;

    bool all_elks_ready = false;

    place->lock();
    cout << "Elk " << id << ": back from holidays." << endl;
    place->available_elks.push_back(this);
    if (place->available_elks.size() == 9)
    {
      all_elks_ready = true;
      cout << "Elk " << id << ": will wake up santa." << endl;
    }
    place->unlock();

    if (all_elks_ready)
      santa->signal(santa_claus::elks_ready);

    if (await() == -1)
      return;

#if 0
    place->lock();
    cout << "Elk " << id << ": yeeehaw!." << endl;
    place->unlock();
#endif
  }
}

void santa_claus::work()
{
  while(true)
  {
    place->lock();
    cout << "Hohoho! Time to take a nap!" << endl;
    place->unlock();

    int signal = await_any();

    if (signal == -1)
      return;

    switch(signal)
    {
    case elks_ready:
    {
      std::vector<elk*> present_elks;
      place->lock();
      cout << "Hohoho! Let's' deliver presents!" << endl;
      present_elks = std::move(place->available_elks);
      place->unlock();

      for (elk *e : present_elks)
        e->signal();

      break;
    }
    case elves_need_help:
    {
      std::vector<elf*> present_elves;
      place->lock();
      cout << "Hohoho! Let's' meet in my study!" << endl;
      int count = 3;
      while(count--)
      {
        present_elves.push_back(place->queued_elves.front());
        place->queued_elves.pop();
      }
      place->unlock();

      for (elf *e : present_elves)
      {
        place->lock();
        cout << "Hohoho! Serving elf: " << e->id << endl;
        place->unlock();
        e->signal();
        if (await(elf_finished_consulting) == -1)
          return;
      }

      break;
    }
    }
  }
}

int main()
{
  santas_place place;
  santa_claus santa(&place);

  elf *elves[10];
  elk *elks[9];

  for(int i = 0; i < 10; ++i)
  {
    elves[i] = new elf(i, &place, &santa);
    elves[i]->go();
  }

  for(int i = 0; i < 9; ++i)
  {
    elks[i] = new elk(i, &place, &santa);
    elks[i]->go();
  }

  santa.go();

  this_thread::sleep_for(milliseconds(2000));

  for(elf *e : elves)
    e->stop();
  for(elk *e : elks)
    e->stop();
  santa.stop();

  for(elf *e : elves)
    e->join();
  for(elk *e : elks)
    e->join();
  santa.join();
}
