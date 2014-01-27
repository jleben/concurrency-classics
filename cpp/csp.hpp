#include <mutex>
#include <condition_variable>
#include <thread>

using namespace std;

class process;
template <typename T> class notifier;

struct dummy_channel_type {};

template<typename T>
class channel
{
  friend class process;
  friend class notifier<T>;

  mutex m_mutex;
  condition_variable m_response;
  bool m_expected;
  bool m_signalled;
  T m_value;

public:
  channel():
    m_expected(false),
    m_signalled(false)
  {}
};

class signal : public channel<dummy_channel_type> {};

class process
{
  template<typename T> friend class channel_writer;

  mutex m_mutex;
  condition_variable m_signal;
  thread m_thread;

public:
  void go()
  {
    m_thread = thread(&process::work, this);
  }

  void join()
  {
    m_thread.join();
  }

protected:
  virtual void work() = 0;

  template<typename ... Types>
  int await( channel<Types> & ... channels)
  {
    unique_lock<mutex> process_lock(m_mutex);
    set_channels_expected(true, channels...);
    int signalled_idx = -1;
    while( (signalled_idx = signalled_channel(0,channels...)) == -1 )
      m_signal.wait(process_lock);
    return signalled_idx;
  }


  template <typename T>
  T await( channel<T> & ch )
  {
    T value;
    unique_lock<mutex> process_lock(m_mutex);
    ch.m_expected = true;
    while (!ch.m_signalled)
      m_signal.wait(process_lock);
    value = ch.m_value;
    ch.m_signalled = false;
    ch.m_expected = false;
    ch.m_response.notify_one();
    return value;
  }

  template <typename T>
  void notify( channel<T> & ch, const T& value = T() )
  {
    unique_lock<mutex> channel_lock(ch.m_mutex);
    unique_lock<mutex> process_lock(m_mutex);
    ch.m_signalled = true;
    ch.m_value = value;
    if (ch.m_expected)
      m_signal.notify_one();
    while(ch.m_signalled)
      ch.m_response.wait(process_lock);
  }

private:
  void set_channels_expected(bool expected)
  {}

  template<typename T, typename ... Types>
  void set_channels_expected(bool expected, channel<T> & ch, Types & ... channels)
  {
    ch.m_expected = expected;
    set_channels_expected(expected, channels...);
  }

  int signalled_channel(int idx)
  {
    return -1;
  }

  template<typename T, typename ... Types>
  int signalled_channel(int idx, channel<T> & ch, Types & ... channels)
  {
    if (ch.m_signalled)
      return idx;
    ++idx;
    return signalled_channel(idx, channels...);
  }
};


template <typename T>
class notifier
{
  channel<T> *m_channel;
  process *m_process;

public:
  notifier( channel<T> *ch, process * p ):
    m_channel(ch),
    m_process(p)
  {}

  void signal( const T& value = T() )
  {
    m_process->notify(*m_channel, value);
  }
};

template <typename T>
notifier<T> notifier_for( channel<T> *ch, process * p)
{
  return notifier<T>(ch, p);
}
