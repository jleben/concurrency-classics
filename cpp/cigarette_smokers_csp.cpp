#include "csp.hpp"
#include "util.hpp"

#include <cstdio>
#include <chrono>
#include <array>
#include <random>

using namespace std;
using namespace std::chrono;

class smoker : public process
{
  signal m_ingredients;
  int m_index;
public:
  smoker(int idx): m_index(idx) {}
  void give_ingredients() { notify(m_ingredients); }
private:
  void work()
  {
    while(true)
    {
      //PRINT("Smoker " << m_index << " waiting.");
      await(m_ingredients);
      PRINT("Smoker " << m_index << " smoking.");
      this_thread::sleep_for(milliseconds(200));
    }
  }
};

class dealer : public process
{
  array<smoker*,3> smokers;
public:
  dealer(smoker *s1, smoker *s2, smoker *s3)
  {
    smokers[0] = s1;
    smokers[1] = s2;
    smokers[2] = s3;
  }
private:
  void work()
  {
    random_device rd;
    minstd_rand R(rd());
    uniform_int_distribution<int> U(0,2);

    while(true)
    {
      int i = U(R);
      smokers[i]->give_ingredients();
    }
  }
};

int main()
{
  smoker s1(1);
  smoker s2(2);
  smoker s3(3);
  dealer d { &s1, &s2, &s3 };

  s1.go();
  s2.go();
  s3.go();
  d.go();

  d.join();
}
