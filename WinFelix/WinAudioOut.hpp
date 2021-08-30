#pragma once

#include "Utility.hpp"

class Core;
class IEncoder;

class WinAudioOut
{
public:

  WinAudioOut();
  ~WinAudioOut();

  void setEncoder( std::shared_ptr<IEncoder> pEncoder );
  void fillBuffer( std::span<std::shared_ptr<Core> const> instances, int64_t renderingTime );

private:

  ComPtr<IMMDevice> mDevice;
  ComPtr<IAudioClient> mAudioClient;
  ComPtr<IAudioClock> mAudioClock;
  ComPtr<IAudioRenderClient> mRenderClient;
  std::shared_ptr<IEncoder> mEncoder;
  HANDLE mEvent;


  boost::rational<int64_t> mFrequency;
  boost::rational<int64_t> mQPCRatio;

  uint32_t mBufferSize;
  std::vector<AudioSample> mSamplesBuffer;

  WAVEFORMATEX * mMixFormat;
  int64_t mSamplesDelta;
  int64_t mSamplesDeltaDelta;
};
