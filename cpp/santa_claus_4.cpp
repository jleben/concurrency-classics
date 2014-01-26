#include "csp.hpp"

#include <cstdio>
#include <chrono>
#include <array>

using namespace std;
using namespace std::chrono;

class santa_claus;
class elf_barrier;
class elk_barrier;

class elf : public process
{
  santa_claus * m_santa;
  elf_barrier * m_barrier;
  signal m_help_started;
  signal m_help_done;

public:
  const int idx;

  elf (int idx, santa_claus *s, elf_barrier *b):
    idx(idx),
    m_santa(s),
    m_barrier(b)
  {}

  void help()
  {
    notify(m_help_started);
  }

  void notify_help_done()
  {
    notify(m_help_done);
  }

private:
  void work();
};

class elk : public process
{
  santa_claus * m_santa;
  elk_barrier * m_barrier;
  signal m_delivery_started;
  signal m_delivery_done;

public:
  elk (santa_claus *s, elk_barrier *b): m_santa(s), m_barrier(b) {}

  void ride()
  {
    notify(m_delivery_started);
  }

  void notify_delivery_done()
  {
    notify(m_delivery_done);
  }

private:
  void work();
};

class santa_claus : public process
{
public:
  typedef array<elf*,3> elf_group;
  typedef array<elk*,9> elk_group;

private:
  channel<elf_group> m_elves;
  channel<elk_group> m_elks;

public:
  santa_claus()
  {
  }

  void help_elves( const elf_group & group )
  {
    notify(m_elves, group);
  }

  void deliver_presents( const elk_group & group )
  {
    notify(m_elks, group);
  }

  void work();
};

class elf_barrier : public process
{
  channel<elf*> m_ready_elves;
  santa_claus *m_santa;

public:
  elf_barrier(santa_claus *s):
    m_santa(s)
  {}

  void enqueue(elf *e)
  {
    notify(m_ready_elves, e);
  }

private:
  void work()
  {
    santa_claus::elf_group group;
    while(true)
    {
      for (int idx = 0; idx < group.size(); ++idx)
      {
        group[idx] = await(m_ready_elves);
        printf("Elf %i (%i) ready\n", group[idx]->idx, idx);
      }
      m_santa->help_elves(group);
    }
  }
};

class elk_barrier : public process
{
  channel<elk*> m_ready_elks;
  santa_claus *m_santa;

public:
  elk_barrier(santa_claus *s):
    m_santa(s)
  {}

  void enqueue(elk *e)
  {
    notify(m_ready_elks, e);
  }

private:
  void work()
  {
    santa_claus::elk_group group;
    while(true)
    {
      for (int idx = 0; idx < group.size(); ++idx)
      {
        group[idx] = await(m_ready_elks);
        printf("Elk %i ready\n", idx);
      }
      m_santa->deliver_presents(group);
    }
  }
};

void elf::work()
{
  while(true)
  {
    m_barrier->enqueue(this);
    await(m_help_started);
    printf("Elf %i getting helped.\n", idx);
    await(m_help_done);
    this_thread::sleep_for(milliseconds(100));
  }
}

void elk::work()
{
  while(true)
  {
    m_barrier->enqueue(this);
    await(m_delivery_started);
    printf("Elk: ye ye yeeehaw!\n");
    await(m_delivery_done);
    this_thread::sleep_for(milliseconds(600));
  }
}

void santa_claus::work()
{
  while(true)
  {
    printf("Hohoho! Let's try to get some sleep...\n");

    int signalled = await(m_elks, m_elves);
    switch (signalled)
    {
    case 0:
    {
      elk_group elks = await(m_elks);
      printf("Hohoho! Got elks!\n");
      for (elk * e : elks)
        e->ride();
      for (elk * e : elks)
        e->notify_delivery_done();
      break;
    }
    case 1:
    {
      elf_group elves = await(m_elves);
      printf("Hohoho! Got elves!\n");
      for (elf *e : elves)
        e->help();
      for (elf *e : elves)
        e->notify_help_done();
      break;
    }
    default:
      printf("Hohoho! There must be an error!\n");
    }
  }
}

int main()
{
  santa_claus santa;
  elf_barrier elf_b(&santa);
  elk_barrier elk_b(&santa);

  int elf_count = 7;
  while(elf_count--)
  {
    elf *e = new elf(elf_count, &santa, &elf_b);
    e->go();
  }

  int elk_count = 9;
  while(elk_count--)
  {
    elk * e = new elk(&santa, &elk_b);
    e->go();
  }

  elf_b.go();
  elk_b.go();
  santa.go();

  this_thread::sleep_for(milliseconds(2000));
}
