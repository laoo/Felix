#pragma once

#include <Windows.h>
#include <AudioClient.h>
#include <MMDeviceAPI.h>
#include <atlbase.h>

#include <vector>
#include <deque>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>
#include <utility>

class WinAudioOut
{
public:

  WinAudioOut( uint32_t audioBufferLengthMs );
  ~WinAudioOut();

  void fillBuffer( std::function<std::pair<float, float>( int sps )> & fun );

private:

  ComPtr<IMMDevice> mDevice;
  ComPtr<IAudioClient> mAudioClient;
  ComPtr<IAudioRenderClient> mRenderClient;
  ComPtr<IAudioClock> mAudioClock;

  UINT64 mDeviceFrequency;
  uint32_t mBufferSize;

  WAVEFORMATEX * mMixFormat;
};
