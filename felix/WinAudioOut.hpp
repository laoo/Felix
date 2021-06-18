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

#include "Utility.hpp"

class Core;

class WinAudioOut
{
public:

  WinAudioOut();
  ~WinAudioOut();

  void fillBuffer( std::span<std::shared_ptr<Core> const> instances );

private:

  ComPtr<IMMDevice> mDevice;
  ComPtr<IAudioClient> mAudioClient;
  ComPtr<IAudioRenderClient> mRenderClient;
  HANDLE mEvent;

  uint32_t mBufferSize;
  std::vector<AudioSample> mSamplesBuffer;

  WAVEFORMATEX * mMixFormat;
};
