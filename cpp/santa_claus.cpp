
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




struct problem
{
  minstd_rand R;
  uniform_int_distribution<int> U_elf_interarrival_time;
  uniform_int_distribution<int> U_elk_interarrival_time;

  mutex mux;
  condition_variable elf_wait_queue;
  condition_variable elf_santa_queue;
  condition_variable elk_queue;
  condition_variable santa_alarm;

  int elves_waiting;
  int elves_allowed;
  int elves_serviced;
  bool elves_with_santa;
  vector<int> elve_ids_with_santa;

  int elks_present;

  problem():
    U_elf_interarrival_time(100,200),
    U_elk_interarrival_time(200,300),
    elves_waiting(0),
    elves_allowed(0),
    elves_serviced(0),
    elves_with_santa(false),
    elks_present(0)
  {}

  void elf()
  {
    int id = R();

    while(true)
    {
      this_thread::sleep_for(milliseconds(U_elf_interarrival_time(R)));

      unique_lock<mutex> lock(mux);

      elves_waiting++;

      if (elves_with_santa || elves_waiting < 3)
      {
        while(!elves_allowed)
        {
          elf_wait_queue.wait(lock);
        }
      }
      else
      {
        elves_allowed += 3;
        elves_with_santa = true;
        elf_wait_queue.notify_one();
        elf_wait_queue.notify_one();
      }

      --elves_waiting;
      --elves_allowed;

      elve_ids_with_santa.push_back(id);
      cout << "Elf " << id << ": Please, help me with this toy..." << endl;

      if (!elves_allowed)
          santa_alarm.notify_one();

      elf_santa_queue.wait(lock);

      ++elves_serviced;

      if (elves_serviced == 3)
      {
        elves_serviced = 0;
        elves_with_santa = false;
        elf_wait_queue.notify_all();
      }
    }
  }

  void elk()
  {
    while (true)
    {
      this_thread::sleep_for(milliseconds(U_elk_interarrival_time(R)));

      unique_lock<mutex> lock(mux);

      cout << "Elk: Ooooh, back at south pole. The holidays were aaaawesome!" << endl;

      ++elks_present;

      if (elks_present == 9)
        santa_alarm.notify_one();

      while (elks_present)
        elk_queue.wait(lock);
    }
  }

  void santa()
  {
    unique_lock<mutex> lock(mux);

    while (true)
    {
      while (elks_present < 9 && elve_ids_with_santa.size() < 3)
      {
        cout << "Santa: Aaaalright, taking a nap..." << endl;
        santa_alarm.wait(lock);
      }

      if (elks_present == 9)
      {
        int idx = 0;
        for (int idx = 0; idx < elks_present; ++idx)
          cout << "Hello my big elk number " << (idx) << "!" << endl;

        cout << "Let's get those gifts to kids!" << endl;

        cout << "Hohoho, back at south pole!" << endl;

        elks_present = 0;
        elk_queue.notify_all();
      }

      if (elve_ids_with_santa.size() == 3)
      {
        cout << "Hohoho, welcome, my little elves!" << endl;

        for (int elf_id : elve_ids_with_santa)
          cout << "Hi elf " << elf_id << ". Build this toy like this and like that!" << endl;

        elve_ids_with_santa.clear();

        elf_santa_queue.notify_all();
      }
    }
  }

};


int main()
{
  problem p;

  unique_lock<mutex> lock(p.mux);

  thread santa_thread(&problem::santa, &p);
  thread elf_threads[10];
  thread elk_threads[9];

  int elf_count = 10;
  while(elf_count--)
  {
    elf_threads[elf_count] = thread(&problem::elf, &p);
  }

  int elk_count = 9;
  while (elk_count--)
    elk_threads[elk_count] = thread(&problem::elk, &p);

  //this_thread::sleep_for(milliseconds(100));

  int iterations = 100;
  while(iterations--)
  {
    p.elf_santa_queue.wait(lock);
  }
}
