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

  int correctedSPS( int64_t samplesEmittedPerFrame, int64_t renderingTimeQPC );

  ComPtr<IMMDevice> mDevice;
  ComPtr<IAudioClient> mAudioClient;
  ComPtr<IAudioClock> mAudioClock;
  ComPtr<IAudioRenderClient> mRenderClient;
  std::shared_ptr<IEncoder> mEncoder;
  HANDLE mEvent;

  double mTimeToFrames;

  uint32_t mBufferSize;
  std::vector<AudioSample> mSamplesBuffer;

  WAVEFORMATEX * mMixFormat;
  int32_t mSamplesDelta;
  int32_t mSamplesDeltaDelta;
};
