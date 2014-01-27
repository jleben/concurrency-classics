#include "csp.hpp"
#include "util.hpp"

#include <cstdio>
#include <chrono>
#include <array>
#include <random>

using namespace std;
using namespace std::chrono;

class room;
class customer;
class barber;

atomic<int> g_arrived_customer_count(0);
atomic<int> g_done_customer_count(0);
constexpr int g_total_customer_count(10000);
constexpr int g_max_queue_size(10);
constexpr int g_thread_count(20);

class customer : public process
{
  room *m_room;
  barber *m_barber;
  channel<bool> m_can_enter;
  signal m_cutting_done;
  int m_index;

public:
  customer(barber *b, room *r): m_barber(b), m_room(r) {}
  void notify_can_enter(bool ok) { notify(m_can_enter, ok); }
  void notify_cutting_done() { notify(m_cutting_done); }
  int index() { return m_index; }
private:
  void work();
};

class barber : public process
{
  room *m_room;
  channel<customer*> m_next_customer;
public:
  barber(room* r): m_room(r) {}
  void notify_next_customer( customer * c ) { notify(m_next_customer, c); }
private:
  void work();
};


class room : public process
{
  channel<customer*> m_new_customer;
  signal m_barber_free;

public:
  void notify_new_customer(customer *c) { notify(m_new_customer, c); }
  void notify_barber_free() { notify(m_barber_free); }
private:
  void work();
};

void customer::work()
{
  while (true)
  {
    int index = m_index = g_arrived_customer_count++;

    m_room->notify_new_customer(this);
    //PRINT("Customer " << index << " looking for chair.");
    bool can_enter = await(m_can_enter);
    if (can_enter)
    {
      PRINT("Customer " << index << " sitting in the lobby.");
      m_barber->notify_next_customer(this);
      //PRINT("Customer " << index << " getting haircut.");
      await(m_cutting_done);
      PRINT("Customer " << index << " done.");
    }
    else
    {
      PRINT("Customer " << index << " rejected.");
      //this_thread::sleep_for(milliseconds(100));
    }
  }
}

void room::work()
{
  const int max_queue_size = g_max_queue_size;
  int queue_size = 0;

  while (true)
  {
    int sig = await(m_barber_free, m_new_customer);
    if (sig == 0)
    {
      await(m_barber_free);
      --queue_size;
      PRINT("Room: -- Queue size = " << queue_size);
    }
    else
    {
      customer *c = await(m_new_customer);
      if (queue_size < max_queue_size)
      {
        c->notify_can_enter(true);
        ++queue_size;
        PRINT("Room: ++ Queue size = " << queue_size);
      }
      else
      {
        c->notify_can_enter(false);
      }
    }
  }
}

void barber::work()
{
  while (g_done_customer_count < g_total_customer_count)
  {
    customer *c = await(m_next_customer);
    PRINT("Barber: Cutting customer: " << c->index());
    c->notify_cutting_done();
    m_room->notify_barber_free();
    PRINT("Barber: Done customer: " << g_done_customer_count);
    ++g_done_customer_count;
  }
}

int main()
{
  room r;
  barber b(&r);

  r.go();
  b.go();

  int thread_count = g_thread_count;
  while(thread_count--)
  {
    customer *c = new customer(&b, &r);
    c->go();
  }

  high_resolution_clock::time_point start_time = high_resolution_clock::now();
  b.join();
  high_resolution_clock::time_point end_time = high_resolution_clock::now();

  duration<double,milli> total_time = end_time - start_time;
  cout << "Total time = " << total_time.count() << endl;
}
