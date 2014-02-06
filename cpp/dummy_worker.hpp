#include <random>

class dummy_worker
{
  std::random_device rd;
  std::minstd_rand R;
  long count;
public:
  dummy_worker(long count = 1e3): R(rd()), count(count) {}
  void work()
  {
    int c = count;
    int result;
    while(c--)
    {
      result = R() * R();
    }
  }
};
