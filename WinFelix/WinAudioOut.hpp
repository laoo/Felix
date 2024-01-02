#pragma once

#include "Utility.hpp"
#include "wav.h"

class Core;
class IEncoder;

class WinAudioOut
{
public:

  WinAudioOut();
  ~WinAudioOut();

  void setEncoder( std::shared_ptr<IEncoder> pEncoder );
  bool wait();
  CpuBreakType fillBuffer( std::shared_ptr<Core> instance, int64_t renderingTime, RunMode runMode );
  void setWavOut( std::filesystem::path path );
  bool isWavOut() const;
  void mute( bool value );
  bool mute() const;

private:

  int correctedSPS( int64_t samplesEmittedPerFrame, int64_t renderingTimeQPC );

  ComPtr<IMMDevice> mDevice;
  ComPtr<IAudioClient> mAudioClient;
  ComPtr<IAudioClock> mAudioClock;
  ComPtr<IAudioRenderClient> mRenderClient;
  std::shared_ptr<IEncoder> mEncoder;
  WavFile* mWav;
  HANDLE mEvent;
  mutable std::mutex mMutex;

  double mTimeToSamples;

  uint32_t mBufferSize;
  std::vector<AudioSample> mSamplesBuffer;

  WAVEFORMATEX * mMixFormat;
  int32_t mSamplesDelta;
  int32_t mSamplesDeltaDelta;

  std::array<int, 32> mWindow;

  float mNormalizer;
};
