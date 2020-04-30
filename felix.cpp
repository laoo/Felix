#include <iostream>
#include <experimental/coroutine>

struct Sus
{
  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  Sus( handle c ) : coro{ c }
  {
  }

  ~Sus()
  {
    if ( coro )
      coro.destroy();
  }

  struct promise_type
  {
    auto get_return_object() { return Sus{ handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_never{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void() {}
  };

  handle coro;
};


struct Dispatcher
{
  struct D
  {

    Dispatcher * disp;


  } mD;

  void setValue( int v )
  {
    mValue = v;
    mSet = true;
    if ( coro )
      coro.resume();
  }

  int mValue;
  bool mSet;

  Dispatcher() : mValue{ 0 }, mSet{ false }
  {

  }

  bool await_ready() { return mSet; }
  int await_resume() { return mValue; }
  void await_suspend( std::experimental::coroutine_handle<> c )
  {
    coro = c;
  }

  std::experimental::coroutine_handle<> coro;
};



Sus sus( Dispatcher & d )
{
  int v = co_await d;
  std::cout << v << std::endl;
}



struct Test
{
  struct promise_type;
  using handle = std::experimental::coroutine_handle<promise_type>;

  struct promise_type
  {
    int value;
    auto get_return_object() { return Test{ handle::from_promise( *this ) }; }
    auto initial_suspend() { return std::experimental::suspend_never{}; }
    auto final_suspend() noexcept { return std::experimental::suspend_always{}; }
    void return_void() {}
    auto & yield_value( int v )
    {
      value = v;
      return *this;
    }
    int await_resume()
    {
      return value;
    }

    bool await_ready() { return false; }
    void await_suspend( std::experimental::coroutine_handle<> ) {}


  };

  Test( handle c ) : coro{ c }
  {
  }

  ~Test()
  {
    if ( coro )
      coro.destroy();
  }

  int set( int value )
  {
    auto & promise = coro.promise();
    auto result = promise.value;
    promise.value = value;
    coro.resume();
    return result;
  }

  handle coro;
};

Test ytest( int v )
{
  int w = co_yield v * 2;
  std::cout << "IN " << w << std::endl;
}


int main()
{
  {
    Dispatcher d{};
    Sus s = sus( d );
    d.setValue( 42 );
  }

  {
    int w;
    {
      auto cr = ytest( 21 );
      w = cr.set( 10 );
    }
    std::cout << "OUT " << w << std::endl;

  }
  return 0;
}
