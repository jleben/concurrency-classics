#include "util.hpp"
#include "csp.hpp"

#include <chrono>
#include <random>
#include <atomic>

using namespace std;
using namespace std::chrono;

atomic<unsigned int> water_count(0);
constexpr int max_water_count = 10000;

struct metric
{
  high_resolution_clock::time_point start_time, stop_time;
};

metric metrics[max_water_count];

enum element_type
{
  hydrogen,
  oxygen
};

class h2o;

class element : public process
{
  h2o * m_water;
  element_type m_type;

  void work();

public:
  element( h2o * water ):
    m_water(water)
  {}
  element( element_type type, h2o * water ):
    m_water(water),
    m_type(type)
  {}
};

class h2o : public process
{
private:
  signal m_ready_hydrogen;
  signal m_ready_oxygen;
  signal m_bonded;

public:
  void notify_hydrogen() { notify(m_ready_hydrogen); }
  void notify_oxygen() { notify(m_ready_oxygen); }
  void notify_bonded() { notify(m_bonded); }
  void work();
};

void element::work()
{
#if 0
  random_device rd;
  minstd_rand R(rd());
  uniform_int_distribution<int> U(0,2);

  int r = U(R);
  element_type type = r == 0 ? oxygen : hydrogen;
#endif

  element_type type = m_type;

  while (water_count < max_water_count)
  {
    //PRINT("New element: " << (type == oxygen ? "O" : "H"));

    if (type == hydrogen)
    {
      m_water->notify_hydrogen();
      PRINT("H bonding.");
    }
    else
    {
      m_water->notify_oxygen();
      PRINT("O bonding.");
    }

    m_water->notify_bonded();
  }
}

void h2o::work()
{
  while (water_count < max_water_count)
  {
    metrics[water_count].start_time = high_resolution_clock::now();
    await(m_ready_oxygen);
    await(m_ready_hydrogen);
    await(m_ready_hydrogen);
    int count = 3;
    while(count--)
      await(m_bonded);
    PRINT("Got water!");
    metrics[water_count].stop_time = high_resolution_clock::now();
    ++water_count;
  }
}


int main()
{
  h2o water;
  water.go();

  int oxygen_count = 50;
  while(oxygen_count--)
  {
    element * e = new element(oxygen, &water);
    e->go();
  }

  int hydrogen_count = 100;
  while(hydrogen_count--)
  {
    element * e = new element(hydrogen, &water);
    e->go();
  }

  high_resolution_clock::time_point start_time = high_resolution_clock::now();
  water.join();
  high_resolution_clock::time_point end_time = high_resolution_clock::now();

  duration<double,milli> total_time = end_time - start_time;
  //duration<double,milli> total_time =
      //metrics[max_water_count-1].stop_time - metrics[0].stop_time;

  //PRINT("Total time = " << total_time.count());
  cout << "Total time = " << total_time.count() << endl;
}
