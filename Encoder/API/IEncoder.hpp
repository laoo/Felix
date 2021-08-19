#pragma once

#include <cstdint>

class IEncoder
{
public:
  virtual ~IEncoder() = default;

  virtual uint32_t width() const = 0;
  virtual uint32_t height() const = 0;
  virtual uint32_t vscale() const = 0;

  virtual void startEncoding( int fpsNumerator, int fpsDenominator ) = 0;
  virtual bool writeFrame( uint8_t const* y, int ystride, uint8_t const* u, int ustride, uint8_t const* v, int vstride ) = 0;
  virtual void pushAudioBuffer( std::span<float const> buf ) = 0;
};

#ifdef ENCODER_EXPORTS
# define ENCODER_EXPORT __declspec(dllexport)
#else
# define ENCODER_EXPORT __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef IEncoder* ( *PCREATE_ENCODER )( char const* path, int vbitrate, int abitrate, int width, int height );
typedef void ( *PDISPOSE_ENCODER )( IEncoder* encoder );

ENCODER_EXPORT IEncoder* createEncoder( char const* path, int vbitrate, int abitrate, int width, int height );
ENCODER_EXPORT void disposeEncoder( IEncoder* encoder );

#ifdef __cplusplus
}
#endif
