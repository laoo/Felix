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

class Felix;

class WinAudioOut
{
public:

  WinAudioOut();
  ~WinAudioOut();

  void fillBuffer( Felix & felix );

private:

  ComPtr<IMMDevice> mDevice;
  ComPtr<IAudioClient> mAudioClient;
  ComPtr<IAudioRenderClient> mRenderClient;
  HANDLE mEvent;

  uint32_t mBufferSize;

  WAVEFORMATEX * mMixFormat;
};
