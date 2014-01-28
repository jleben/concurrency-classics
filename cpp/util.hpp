#include <mutex>
#include <atomic>
#include <iostream>
#include <random>

class printer
{
  static std::mutex & the_mutex()
  {
    static std::mutex s_mutex;
    return s_mutex;
  }

public:
  printer()
  {
    the_mutex().lock();
  }

  ~printer()
  {
    std::cout << std::endl;
    the_mutex().unlock();
  }

  template <typename T>
  printer & operator<<( const T & data )
  {
    std::cout << data;
    return *this;
  }
};


#ifdef NDEBUG
#define PRINT(x)
#else
#define PRINT(x) printer() << x
#endif

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
