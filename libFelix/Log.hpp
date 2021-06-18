#pragma once


class Log
{
public:
  enum LogLevel
  {
    LL_TRACE,
    LL_DEBUG,
    LL_INFO,
    LL_NOTICE,
    LL_WARNING,
    LL_ERROR
  };

  void setLogLevel( LogLevel ll );

  void log( LogLevel ll, std::string const& message );

  static Log & instance();

private:
  Log();

  LogLevel mLogLevel;
};

class Formatter
{
public:
  Formatter( Log::LogLevel ll );
  ~Formatter();

  template<typename T>
  Formatter & operator<<( T const& t )
  {
    mSS << t;
    return *this;
  }

private:
  Log::LogLevel mLl;
  std::stringstream mSS;
};

#define L_TRACE ::Formatter{ ::Log::LL_TRACE }
#define L_DEBUG ::Formatter{ ::Log::LL_DEBUG }
#define L_INFO ::Formatter{ ::Log::LL_INFO }
#define L_NOTICE ::Formatter{ ::Log::LL_NOTICE }
#define L_WARNING ::Formatter{ ::Log::LL_WARNING }
#define L_ERROR ::Formatter{ ::Log::LL_ERROR }

#define L_SET_LOGLEVEL(LL) ::Log::instance().setLogLevel( LL );
