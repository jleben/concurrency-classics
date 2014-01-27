#include <mutex>
#include <atomic>
#include <iostream>

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
