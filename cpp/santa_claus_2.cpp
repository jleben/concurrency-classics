
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
  condition_variable m_signal;
  thread m_thread;
  bool m_run;
  bool m_signalled;

public:
  process():
    m_run(false),
    m_signalled(false)
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
    m_signalled = true;
    m_signal.notify_one();
  }

  void signal()
  {
    unique_lock<mutex> public_lock(m_mutex);
    m_signalled = true;
    m_signal.notify_one();
  }

  void join()
  {
    m_thread.join();
  }

protected:
  virtual void work() = 0;

  bool wait()
  {
    unique_lock<mutex> lock(m_mutex);
    while(m_run && !m_signalled)
    {
      m_signal.wait(lock);
    }
    m_signalled = false;
    return m_run;
  }

  template< class Rep, class Period >
  bool sleep_for( const std::chrono::duration<Rep,Period>& sleep_duration )
  {
    this_thread::sleep_for(sleep_duration);
    return m_run;
  }

private:
  virtual void run()
  {
    work();
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
  santas_place *place;
  santa_claus *santa;
  santa_claus( santas_place *p ):
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

    place->lock();
    place->queued_elves.push(this);
    if (place->queued_elves.size() >= 3 && !place->elves_in_service_count)
    {
      place->elves_in_service_count = 3;
      santa->signal();
    }
    place->unlock();

    if (!wait())
      return;

    place->lock();
    cout << "Elf " << id << ": got help." << endl;
    --place->elves_in_service_count;
    place->unlock();

    santa->signal(); // Done getting help;
  }
}

void elk::work()
{
  while(true)
  {
    if (!sleep_for(milliseconds(300)))
      return;

    place->lock();
    cout << "Elk " << id << ": back from holidays." << endl;
    place->available_elks.push_back(this);
    if (place->available_elks.size() == 9)
      santa->signal();
    place->unlock();

    if (!wait())
      return;

    place->lock();
    cout << "Elk " << id << ": yeeehaw!." << endl;
    place->unlock();
  }
}

void santa_claus::work()
{
  while(true)
  {
    if (!wait())
      return;

    std::vector<elk*> present_elks;
    std::vector<elf*> present_elves;

    place->lock();

    if (place->available_elks.size() == 9)
      present_elks = std::move(place->available_elks);
    else if (place->queued_elves.size() >= 3)
    {
      int count = 3;
      while(count--)
      {
        present_elves.push_back(place->queued_elves.front());
        place->queued_elves.pop();
      }
    }

    place->unlock();

    if (present_elks.size())
    {
      place->lock();
      cout << "Hohoho! Let's' deliver presents!" << endl;
      place->unlock();

      for (elk *e : present_elks)
        e->signal();
    }

    if (present_elves.size())
    {
      place->lock();
      cout << "Hohoho! Let's' meet in my study!" << endl;
      place->unlock();

      for (elf *e : present_elves)
      {
        place->lock();
        cout << "Serving elf: " << e->id << endl;
        place->unlock();
        e->signal();
        if (!wait())
          return;
      }
    }
  }
}

int main()
{
  // NOTE: This doesn't work:
  // Santa is not holding his mutex at all times, so he has no way
  // to tell who the signal came from.

  // But is Santa holds the mutex all the time,
  // that complicates the API, makes it asymmetric...

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
