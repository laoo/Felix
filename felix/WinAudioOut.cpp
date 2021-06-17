#include "pch.hpp"
#include "WinAudioOut.hpp"
#include "Felix.hpp"
#include "Log.hpp"

WinAudioOut::WinAudioOut()
{
  CoInitializeEx( NULL, COINIT_MULTITHREADED );

  HRESULT hr;

  ComPtr<IMMDeviceEnumerator> deviceEnumerator;
  hr = CoCreateInstance( __uuidof( MMDeviceEnumerator ), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS( deviceEnumerator.ReleaseAndGetAddressOf() ) );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = deviceEnumerator->GetDefaultAudioEndpoint( eRender, eMultimedia, mDevice.ReleaseAndGetAddressOf() );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mDevice->Activate( __uuidof( IAudioClient ), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<void **>( mAudioClient.ReleaseAndGetAddressOf() ) );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mAudioClient->GetMixFormat( &mMixFormat );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mAudioClient->Initialize( AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 0, 0, mMixFormat, nullptr );
  if ( FAILED( hr ) )
    throw std::exception{};

  mEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
  if ( mEvent == NULL )
    throw std::exception{};

  mAudioClient->SetEventHandle( mEvent );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mAudioClient->GetBufferSize( &mBufferSize );
  if ( FAILED( hr ) )
    throw std::exception{};

  hr = mAudioClient->GetService( __uuidof( IAudioRenderClient ), reinterpret_cast<void **>( mRenderClient.ReleaseAndGetAddressOf() ) );
  if ( FAILED( hr ) )
    throw std::exception{};

  mAudioClient->Start();
}

WinAudioOut::~WinAudioOut()
{
  mAudioClient->Stop();

  if ( mEvent )
  {
    CloseHandle( mEvent );
    mEvent = NULL;
  }

  CoUninitialize();

  if ( mMixFormat )
  {
    CoTaskMemFree( mMixFormat );
    mMixFormat = nullptr;
  }
}

void WinAudioOut::fillBuffer( std::span<std::shared_ptr<Felix> const> instances )
{
  DWORD retval = WaitForSingleObject( mEvent, 1000 );
  if ( retval != WAIT_OBJECT_0 )
    return;

  HRESULT hr;
  uint32_t padding{};
  hr = mAudioClient->GetCurrentPadding( &padding );
  uint32_t framesAvailable = mBufferSize - padding;
  //L_TRACE << "AUD " << mBufferSize << " - " << padding << " = " << framesAvailable;
  if ( framesAvailable > 0 )
  {
    if ( mSamplesBuffer.size() < framesAvailable * instances.size() )
    {
      mSamplesBuffer.resize( framesAvailable * instances.size() );
    }

    for ( size_t i = 0; i < instances.size(); ++i )
    {
      instances[i]->setAudioOut( mMixFormat->nSamplesPerSec, std::span<AudioSample>{ mSamplesBuffer.data() + framesAvailable * i, framesAvailable } );
    }

    for ( int finished = 0; finished < instances.size(); )
    {
      for ( size_t i = 0; i < instances.size(); ++i )
      {
        finished += instances[i]->advanceAudio();
      }
    }

    BYTE *pData;
    hr = mRenderClient->GetBuffer( framesAvailable, &pData );
    if ( FAILED( hr ) )
      return;
    float* pfData = reinterpret_cast<float*>( pData );
    for ( uint32_t i = 0; i < framesAvailable; ++i )
    {
      pfData[i * mMixFormat->nChannels + 0] = mSamplesBuffer[i].left / 32768.0f;
      pfData[i * mMixFormat->nChannels + 1] = mSamplesBuffer[i].right / 32768.0f;
    }
    hr = mRenderClient->ReleaseBuffer( framesAvailable, 0 );
  }
}



