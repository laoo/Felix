#pragma once

#include <filesystem>
#include <span>
#include <mutex>
#include <queue>

#ifdef __cplusplus
extern "C" {
#endif

#define __STDC_CONSTANT_MACROS



#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#ifdef __cplusplus
}
#endif


#ifdef __cplusplus
extern "C" {
#endif

#define __STDC_CONSTANT_MACROS


#ifdef __cplusplus
}
#endif

#define SAMPLE_RATE 48000 /* 25 images/s */

#include "API/IEncoder.hpp"

class VideoEncoder : public IEncoder
{
public:
  VideoEncoder( std::filesystem::path const& path, int vbitrate, int abitrate, int width, int height );
  ~VideoEncoder() override;

  void pushAudioBuffer( std::span<float const> buf ) override;

  uint32_t width() const override;
  uint32_t height() const override;
  virtual uint32_t vscale() const;

  void startEncoding() override;
  bool writeFrame( uint8_t const* y, int ystride, uint8_t const* u, int ustride, uint8_t const* v, int vstride ) override;

private:
  void openVideo( int width, int height, int bitrate );
  void openAudio( int bitrate );
  int pushFrame( AVCodecContext *c, AVStream *st, AVFrame const* frame );
  static std::shared_ptr<AVFrame> allocVideoFrame( enum AVPixelFormat pix_fmt, int width, int height );
  static std::shared_ptr<AVFrame> allocAudioFrame( enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples );

private:
  struct StreamContext
  {
    AVStream *st;
    std::shared_ptr<AVCodecContext> encoder;

    /* pts of the next frame that will be generated */
    int64_t nextPts;

    std::shared_ptr<AVFrame> frame;
  };

private:
  std::filesystem::path const mPath;
  int const mVbitrate;
  int const mAbitrate;
  AVFormatContext *mFormatContext;
  AVCodec *mAudioCodec;
  AVCodec *mVideoCodec;
  std::shared_ptr<StreamContext> mAudioStream;
  std::shared_ptr<StreamContext> mVideoStream;
  int mWidth;
  int mHeight;

  std::mutex mMutex;
  std::queue<float> mAudioQueue;
};


