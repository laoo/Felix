#include "SuzyExecute.hpp"
#include "BusMaster.hpp"

SuzyExecute suzyExecute( Suzy & suzy )
{
  co_return;
}

AwaitSuzyFetchSCB SuzyExecute::promise_type::yield_value( SuzyFetchSCB f )
{
  return AwaitSuzyFetchSCB{ mBus->request( f ) };
}

AwaitSuzyFetchSprite SuzyExecute::promise_type::yield_value( SuzyFetchSprite f )
{
  return AwaitSuzyFetchSprite{ mBus->request( f ) };
}

AwaitSuzyReadPixel SuzyExecute::promise_type::yield_value( SuzyReadPixel f )
{
  return AwaitSuzyReadPixel{ mBus->request( f ) };
}

AwaitSuzyWritePixel SuzyExecute::promise_type::yield_value( SuzyWritePixel f )
{
  return AwaitSuzyWritePixel{ mBus->request( f ) };
}
