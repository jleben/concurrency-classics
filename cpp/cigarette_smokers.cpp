#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <random>
#include <list>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdint>

using namespace std;
using namespace std::chrono;

atomic<bool> g_run;

enum ingredient
{
  no_ingredient,
  tobacco,
  paper,
  matches,
};

class table
{
  std::mutex m_mux;
  std::condition_variable m_condition;
  ingredient m_ingredients[2];

  bool empty()
  {
    return m_ingredients[0] == no_ingredient &&
        m_ingredients[1] == no_ingredient;
  }

public:
  table()
  {
    m_ingredients[0] = no_ingredient;
    m_ingredients[1] = no_ingredient;
  }

  bool put_ingredients(ingredient a, ingredient b)
  {
    unique_lock<mutex> locker(m_mux);
    if (!empty())
      m_condition.wait(locker, [&](){ return empty() || !g_run; });
    if (!g_run)
      return false;
    m_ingredients[0] = a;
    m_ingredients[1] = b;
    m_condition.notify_all();
    return true;
  }

  bool get_required_ingredients(ingredient stock)
  {
    auto ingredients_available = [&]()
    {
      return
          m_ingredients[0] != no_ingredient && m_ingredients[0] != stock &&
          m_ingredients[1] != no_ingredient && m_ingredients[1] != stock;
    };
    unique_lock<mutex> locker(m_mux);
    if (!ingredients_available())
      m_condition.wait(locker, [&](){ return ingredients_available() || !g_run; });
    if (!g_run)
      return false;
    m_ingredients[0] = no_ingredient;
    m_ingredients[1] = no_ingredient;
    m_condition.notify_all();
    return true;
  }

  void wake_all()
  {
    unique_lock<mutex> locker(m_mux);
    m_condition.notify_all();
  }
};

class agent
{
public:
  agent( table *t ):
    m_table(t),
    m_thread(&agent::run, this)
  {}

  void join() { m_thread.join(); }

private:
  void run()
  {
    minstd_rand gen;
    uniform_int_distribution<int> dis(0, 2);
    vector<ingredient> stock { tobacco, paper, matches };
    while(g_run)
    {
      int i1 = dis(gen);
      int i2;
      do {
        i2 = dis(gen);
      } while(i2 == i1);

      ingredient a = stock[i1];
      ingredient b = stock[i2];

      //cout << "putting ingredients: " << a << " " << b << endl;
      m_table->put_ingredients(a, b);
      //this_thread::sleep_for(milliseconds(30));
    }
  }

  table *m_table;
  thread m_thread;
};

class smoker
{
  ingredient m_stock;
  table *m_table;
  std::uint64_t m_count;
  thread m_thread;

public:
  smoker(ingredient stock, table *t):
    m_stock(stock),
    m_table(t),
    m_count(0),
    m_thread(&smoker::run, this)
  {
  }

  void join()
  {
    m_thread.join();
  }

  std::uint64_t smoked_count() const { return m_count; }

private:
  void run()
  {
    while (g_run)
    {
      if (m_table->get_required_ingredients(m_stock))
        ++m_count;
      //cout << "smoking: " << m_stock << endl;
      //this_thread::sleep_for(milliseconds(10));
    }
  }
};

int main()
{
  table t;

  g_run = true;

  agent a(&t);
  smoker s1(tobacco, &t);
  smoker s2(paper, &t);
  smoker s3(matches, &t);

  this_thread::sleep_for(seconds(1));

  g_run = false;
  t.wake_all();

  cout << "waiting for threads to join..." << endl;

  a.join();
  s1.join();
  s2.join();
  s3.join();

  cout << "smoker 1: " << s1.smoked_count() << endl;
  cout << "smoker 2: " << s2.smoked_count() << endl;
  cout << "smoker 3: " << s3.smoked_count() << endl;
}
