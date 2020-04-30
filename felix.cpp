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



Sus test( Dispatcher & d )
{
  int v = co_await d;
  std::cout << v << std::endl;
}

int main()
{
  Dispatcher d{};

  Sus s = test( d );
  d.setValue( 42 );
  return 0;
}
