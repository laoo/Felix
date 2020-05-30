#include "WinAudioOut.hpp"
#include <stdexcept>
#include <cassert>
#include "Log.hpp"

WinAudioOut::WinAudioOut( uint32_t audioBufferLengthMs )
{
  CoInitializeEx( NULL, COINIT_MULTITHREADED );

  HRESULT hr;


  hr = CoCreateInstance( __uuidof( MMDeviceEnumerator ), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( &mDeviceEnumerator ) );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mDeviceEnumerator->GetDefaultAudioEndpoint( eRender, eMultimedia, &mDevice );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mDevice->Activate( __uuidof( IAudioClient ), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void **>( &mAudioClient ) );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mAudioClient->GetMixFormat( &mMixFormat );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mAudioClient->Initialize( AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_NOPERSIST, audioBufferLengthMs * 10000, 0, mMixFormat, nullptr );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mAudioClient->GetBufferSize( &mBufferSize );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mAudioClient->GetService( __uuidof( IAudioRenderClient ), reinterpret_cast<void **>( &mRenderClient ) );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mAudioClient->GetService( __uuidof( IAudioClock ), reinterpret_cast<void **>( &mAudioClock ) );
  if ( FAILED( hr ) )
    throw std::exception{};

  mAudioClient->Start();
}

WinAudioOut::~WinAudioOut()
{
  mAudioClient->Stop();

  CoUninitialize();

  if ( mMixFormat )
  {
    CoTaskMemFree( mMixFormat );
    mMixFormat = nullptr;
  }
}

void WinAudioOut::fillBuffer( std::function<std::pair<float, float>( int sps )> & fun )
{
  HRESULT hr;

  uint32_t padding{};
  hr = mAudioClient->GetCurrentPadding( &padding );
  uint32_t framesAvailable = mBufferSize - padding;
  if ( framesAvailable > 0 )
  {
    BYTE *pData;
    hr = mRenderClient->GetBuffer( framesAvailable, &pData );
    if ( FAILED( hr ) )
      return;
    float* pfData = reinterpret_cast<float*>( pData );
    for ( uint32_t i = 0; i < framesAvailable; ++i )
    {
      auto pair = fun( mMixFormat->nSamplesPerSec );
      for ( int j = 0; j < mMixFormat->nChannels; ++j )
        pfData[i * mMixFormat->nChannels + j] = ( ( j & 1 ) == 0 ) ? pair.first : pair.second;
    }
    hr = mRenderClient->ReleaseBuffer( framesAvailable, 0 );
    if ( FAILED( hr ) )
      return;
  }
}



