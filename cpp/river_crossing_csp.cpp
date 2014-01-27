#include "util.hpp"
#include "csp.hpp"
#include "river_crossing.hpp"

#include <cstdio>
#include <chrono>
#include <array>
#include <queue>
#include <random>

// solution 2 == deadlock!
#define solution 0

using namespace std;
using namespace std::chrono;

collector metrics;

enum programmer_type
{
  go_programmer,
  pthread_programmer
};

enum boat_role
{
  passenger,
  captain
};

class boat;

class programmer : public process
{
  int m_index;
  programmer_type m_type;
  signal m_can_board;
  channel<boat_role> m_boat_role;
  boat * m_boat;

public:
  programmer( int index, programmer_type t, boat *b ):
    m_index(index),
    m_type(t),
    m_boat(b)
  {}

  void allow_boarding()
  {
    notify(m_can_board);
  }

  void assign_role(boat_role role)
  {
    notify(m_boat_role, role);
  }

  int index() { return m_index; }

  programmer_type type() { return m_type; }

private:
  void work();
};

class boat : public process
{
private:
  channel<programmer*> m_arrived_programmer;
  channel<programmer*> m_boarding_programmer;

  channel<programmer*> m_boarding_go_programmer;
  channel<programmer*> m_boarding_pthread_programmer;

  channel<programmer*> m_queued_go_programmer;
  channel<programmer*> m_queued_pthread_programmer;

public:
  void enqueue(programmer * p)
  {
#if (solution == 0) || (solution == 1)
    notify(m_arrived_programmer, p);
#elif (solution == 2)
    if (p->type() == go_programmer)
      notify(m_queued_go_programmer, p);
    else
      notify(m_queued_pthread_programmer, p);
#endif
  }

  void notify_boarded(programmer *p)
  {
    notify(m_boarding_programmer, p);
  }

  void board(programmer *p)
  {
    if (p->type() == go_programmer)
      notify(m_boarding_go_programmer, p);
    else
      notify(m_boarding_pthread_programmer, p);
  }

private:
  void work();
};

void programmer::work()
{
  random_device rd;
  minstd_rand R(rd());
  uniform_int_distribution<int> U(0,1);

  while(true)
  {
    this_thread::sleep_for(milliseconds(THREAD_SLEEP_TIME));

    m_index = metrics.arrived_passenger_count.fetch_add(1, memory_order_relaxed);

    int passenger_type_coin = U(R);
    m_type = passenger_type_coin == 0 ? go_programmer : pthread_programmer;

    high_resolution_clock::time_point start_time = high_resolution_clock::now();

    m_boat->enqueue(this);
    //PRINT("Programmer: " << m_index << "/" << m_type << " queued.");

#if solution == 0

    boat_role role = await(m_boat_role);

    PRINT("Programmer: " << m_index << "/" << m_type <<   " boarding");

    m_boat->notify_boarded(this);

    if (role == captain)
      PRINT("Captain: " << m_index << "/" << m_type <<   " Let's sail the seas!");

#elif solution == 1

    m_boat->board(this);

    PRINT("Programmer: " << m_index << "/" << m_type <<   " boarding");

    boat_role role = await(m_boat_role);

    if (role == captain)
      PRINT("Captain: " << m_index << "/" << m_type <<   " Let's sail the seas!");
#elif solution == 2

    //PRINT("Programmer: " << m_index << "/" << m_type << " boarding.");

    boat_role role = await(m_boat_role);

    //if (role == captain)
      //PRINT("Captain: " << m_index << "/" << m_type <<   " Let's sail the seas!");

#endif

    high_resolution_clock::time_point end_time = high_resolution_clock::now();

    int departed_passenger_index = metrics.departed_passenger_count.fetch_add(1, memory_order_relaxed);
    if (departed_passenger_index >= TOTAL_PASSENGER_COUNT)
      break;

    passenger_data & metric = metrics.passengers[departed_passenger_index];
    metric.type = (passenger_type) m_type;
    metric.boarded = true;
    metric.start_time = start_time;
    metric.end_time = end_time;

    if (departed_passenger_index == TOTAL_PASSENGER_COUNT - 1)
      metrics.notify_end();
  }
}

#if solution == 0

void boat::work()
{
  queue<programmer*> m_go_queue;
  queue<programmer*> m_pthread_queue;

  vector<programmer*> boatload;
  boatload.reserve(4);


  while(true)
  {
    {
      programmer *p = await(m_arrived_programmer);
      if (p->type() == go_programmer)
        m_go_queue.push(p);
      else
        m_pthread_queue.push(p);
      //PRINT("Boat: programmer arrived: " << p->index() << "/" << p->type());
    }

    if (m_go_queue.size() >= 2 && m_pthread_queue.size() >= 2)
    {
      boatload.push_back(m_go_queue.front()); m_go_queue.pop();
      boatload.push_back(m_go_queue.front()); m_go_queue.pop();
      boatload.push_back(m_pthread_queue.front()); m_pthread_queue.pop();
      boatload.push_back(m_pthread_queue.front()); m_pthread_queue.pop();
    }
    else if (m_go_queue.size() >= 4)
    {
      for(int i = 0; i < 4; ++i)
      {
        boatload.push_back(m_go_queue.front()); m_go_queue.pop();
      }
    }
    else if (m_pthread_queue.size() >= 4)
    {
      for(int i = 0; i < 4; ++i)
      {
        boatload.push_back(m_pthread_queue.front()); m_pthread_queue.pop();
      }
    }
    else
    {
      continue;
    }

    for (int i = 0; i < boatload.size(); ++i)
    {
      programmer *p = boatload[i];
      //PRINT("Boat: assigning role to: " << p->index() << "/" << p->type());
      p->assign_role(i == boatload.size() - 1 ? captain : passenger);
    }

    for (int i = 0; i < boatload.size(); ++i)
      await(m_boarding_programmer);

    boatload.clear();
  }

}

#elif solution == 1

void boat::work()
{
  int m_go_count = 0;
  int m_pthread_count = 0;

  vector<programmer*> boatload;
  boatload.reserve(4);

  while(true)
  {
    {
      programmer *p = await(m_arrived_programmer);

      if (p->type() == go_programmer)
        ++m_go_count;
      else
        ++m_pthread_count;

      //PRINT("Boat: programmer arrived: " << p->index() << "/" << p->type());
    }

    if (m_go_count >= 2 && m_pthread_count >= 2)
    {
      boatload.push_back( await(m_boarding_go_programmer) );
      boatload.push_back( await(m_boarding_go_programmer) );
      boatload.push_back( await(m_boarding_pthread_programmer) );
      boatload.push_back( await(m_boarding_pthread_programmer) );
      m_go_count -= 2;
      m_pthread_count -= 2;
    }
    else if (m_go_count >= 4)
    {
      for(int i = 0; i < 4; ++i)
      {
        boatload.push_back( await(m_boarding_go_programmer) );
      }
      m_go_count -= 4;
    }
    else if (m_pthread_count >= 4)
    {
      for(int i = 0; i < 4; ++i)
      {
        boatload.push_back( await(m_boarding_pthread_programmer) );
      }
      m_pthread_count -= 4;
    }
    else
    {
      continue;
    }

    for (int i = 0; i < boatload.size(); ++i)
    {
      programmer *p = boatload[i];
      //PRINT("Boat: assigning role to: " << p->index() << "/" << p->type());
      p->assign_role(i == boatload.size() - 1 ? captain : passenger);
    }

    boatload.clear();
  }
}
#elif solution == 2

void boat::work()
{
  vector<programmer*> boatload;
  boatload.reserve(4);

  int go_count = 0;
  int pthread_count = 0;

  while(true)
  {
    bool go_allowed = pthread_count ? (go_count < 2) : (go_count < 4);
    bool pthread_allowed = go_count ? (pthread_count < 2) : (pthread_count < 4);

    if (go_allowed && pthread_allowed)
    {
      int result = await(m_queued_go_programmer, m_queued_pthread_programmer);
      if (result == 0)
      {
        boatload.push_back(await(m_queued_go_programmer));
        ++go_count;
      }
      else
      {
        boatload.push_back(await(m_queued_pthread_programmer));
        ++pthread_count;
      }
    }
    else if (go_allowed)
    {
      boatload.push_back(await(m_queued_go_programmer));
      ++go_count;
    }
    else if (pthread_allowed)
    {
      boatload.push_back(await(m_queued_pthread_programmer));
      ++pthread_count;
    }

    if (boatload.size() == 4)
    {
      for (int i = 0; i < boatload.size(); ++i)
      {
        programmer *p = boatload[i];
        //PRINT("Boat: assigning role to: " << p->index() << "/" << p->type());
        p->assign_role(i == boatload.size() - 1 ? captain : passenger);
      }

      PRINT("Boat: Sailing!");

      boatload.clear();
      go_count = 0;
      pthread_count = 0;
    }
  }
}

#endif



int main()
{
  boat b;
  b.go();

  random_device rd;
  minstd_rand R(rd());
  uniform_int_distribution<int> U(0,1);

#if 0
  for(int idx = 0; idx < TOTAL_PASSENGER_COUNT; ++idx)
  {
    int passenger_type_coin = U(R);
    programmer_type type = passenger_type_coin < 1 ? go_programmer : pthread_programmer;

    programmer *p = new programmer(idx, type, &b);
    p->go();

    this_thread::sleep_for(milliseconds(5));
  }
#endif

  for(int idx = 0; idx < THREAD_COUNT; ++idx)
  {
    programmer *p = new programmer(idx, go_programmer, &b);
    p->go();
  }

  metrics.wait_for_end();

  cout << "over" << endl;
  high_resolution_clock::duration avg_time(0);
  high_resolution_clock::duration max_time(0);
  uint64_t time_std = 0;

  size_t boarded_count = 0;

  for (int i = 0; i < TOTAL_PASSENGER_COUNT; ++i)
  {
    passenger_data &p = metrics.passengers[i];
    cout << "Passenger " << i << ":";
    if (p.boarded)
    {
      ++boarded_count;
      auto time = p.end_time - p.start_time;
      avg_time += time;
      if (time > max_time)
        max_time = time;

      duration<double, micro> print_time = time;
      cout << " Time = " << print_time.count();
    }
    else
    {
      cout << " not boarded";
    }
    cout << endl;
  }

  avg_time /= boarded_count;

  for (int i = 0; i < TOTAL_PASSENGER_COUNT; ++i)
  {
    passenger_data &p = metrics.passengers[i];
    if (p.boarded)
    {
      auto time = p.end_time - p.start_time;
      int64_t t = time.count();
      int64_t avg = avg_time.count();
      int64_t d = t - avg;
      time_std += d * d;
    }
  }

  time_std /= boarded_count;
  time_std = std::sqrt(time_std);

  high_resolution_clock::duration total_time =
      metrics.passengers[TOTAL_PASSENGER_COUNT-1].end_time -
      metrics.passengers[0].end_time;

  cout << "Total time = " << duration<double, milli>(total_time).count() << " ms " << endl;
  cout << "Average time = " << duration<double, micro>(avg_time).count() << " us "<< endl;
  cout << "Max time = " << duration<double, micro>(max_time).count() << " us " << endl;
  cout << "Time STD = " << duration<double, micro>( high_resolution_clock::duration(time_std) ).count() << " us " << endl;
}
