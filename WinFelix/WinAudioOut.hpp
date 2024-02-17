#pragma once

#include "Utility.hpp"
#include "wav.h"

class Core;

class WinAudioOut
{
public:

  WinAudioOut();
  ~WinAudioOut();

  bool wait();
  CpuBreakType fillBuffer( std::shared_ptr<Core> instance, int64_t renderingTime, RunMode runMode );
  void setWavOut( std::filesystem::path path );
  bool isWavOut() const;
  void mute( bool value );
  bool mute() const;

private:

  ComPtr<IMMDevice> mDevice;
  ComPtr<IAudioClient> mAudioClient;
  ComPtr<IAudioClock> mAudioClock;
  ComPtr<IAudioRenderClient> mRenderClient;
  WavFile* mWav;
  HANDLE mEvent;
  mutable std::mutex mMutex;

  double mTimeToSamples;

  uint32_t mBufferSize;
  std::vector<AudioSample> mSamplesBuffer;

  WAVEFORMATEX * mMixFormat;
  int32_t mSamplesDelta;
  int32_t mSamplesDeltaDelta;

  float mNormalizer;
};
