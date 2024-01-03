#pragma once

#include "ScriptDebugger.hpp"

class Manager;

struct TrapProxy
{
  std::shared_ptr<ScriptDebuggerEscapes> scriptDebuggerEscapes;
  ScriptDebugger::Type type;

  static void set( TrapProxy & proxy, int idx, sol::function fun );
};

struct RamProxy
{
  Manager& manager;

  sol::object get( sol::stack_object key, sol::this_state L );
  void set( sol::stack_object key, sol::stack_object value, sol::this_state );
};

struct RomProxy
{
  Manager& manager;

  sol::object get( sol::stack_object key, sol::this_state L );
  void set( sol::stack_object key, sol::stack_object value, sol::this_state );
};

struct MikeyProxy
{
  Manager& manager;

  sol::object get( sol::stack_object key, sol::this_state L );
  void set( sol::stack_object key, sol::stack_object value, sol::this_state );
};

struct SuzyProxy
{
  Manager& manager;

  sol::object get( sol::stack_object key, sol::this_state L );
  void set( sol::stack_object key, sol::stack_object value, sol::this_state );
};

struct CPUProxy
{
  Manager& manager;

  sol::object get( sol::stack_object key, sol::this_state L );
  void set( sol::stack_object key, sol::stack_object value, sol::this_state );
};

struct SymbolProxy
{
  Manager& manager;

  sol::object get( sol::stack_object key, sol::this_state L );
};

