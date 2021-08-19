#include "VideoEncoder.hpp"
#include "Ex.hpp"
#include <cassert>

VideoEncoder::VideoEncoder( std::filesystem::path const& path, int vbitrate, int abitrate, int width, int height ) : mPath{ path }, mVbitrate{ vbitrate }, mAbitrate{ abitrate }, mFormatContext{}, mAudioCodec{}, mVideoCodec{}, mWidth{ width }, mHeight{ height }, mMutex{}
{
}

VideoEncoder::~VideoEncoder()
{
  if ( !mFormatContext )
    return;

  pushFrame( &*mVideoStream->encoder, mVideoStream->st, nullptr );
  pushFrame( &*mAudioStream->encoder, mAudioStream->st, nullptr );


  /* Write the trailer, if any. The trailer must be written before you
   * close the CodecContexts open when you wrote the header; otherwise
   * av_write_trailer() may try to use memory that was freed on
   * av_codec_close(). */
  av_write_trailer( mFormatContext );

  if ( !( mFormatContext->oformat->flags & AVFMT_NOFILE ) )
    avio_closep( &mFormatContext->pb );

  /* free the stream */
  avformat_free_context( mFormatContext );
}

void VideoEncoder::pushAudioBuffer( std::span<float const> buf )
{
  std::scoped_lock<std::mutex> l{ mMutex };

  for ( float sample : buf )
  {
    mAudioQueue.push( sample );
  }
}

void VideoEncoder::startEncoding( int fpsNumerator, int fpsDenominator )
{
  AVDictionary *opt = NULL;

  /* allocate the output media context */
  avformat_alloc_output_context2( &mFormatContext, NULL, NULL, mPath.string().c_str() );
  if ( !mFormatContext )
    throw Ex{} << "Could not deduce output format from file extension " << mPath;

  /* Add the audio and video streams using the default format codecs
   * and initialize the codecs. */
  openVideo( mWidth, mHeight, mVbitrate, AVRational{ fpsDenominator, fpsNumerator } );
  openAudio( mAbitrate );

  /* open the output file, if needed */
  if ( !( mFormatContext->oformat->flags & AVFMT_NOFILE ) )
  {
    int ret = avio_open( &mFormatContext->pb, mPath.string().c_str(), AVIO_FLAG_WRITE );
    if ( ret < 0 )
    {
      char errbuf[AV_ERROR_MAX_STRING_SIZE];
      av_strerror( ret, errbuf, AV_ERROR_MAX_STRING_SIZE );
      throw Ex{} << "Could not open " << mPath << " " << errbuf;
    }
  }

  /* Write the stream header, if any. */
  int ret = avformat_write_header( mFormatContext, &opt );
  if ( ret < 0 )
  {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror( ret, errbuf, AV_ERROR_MAX_STRING_SIZE );
    throw Ex{} << "Error occurred when opening output file" << errbuf;
  }
}

uint32_t VideoEncoder::width() const
{
  return mWidth;
}

uint32_t VideoEncoder::height() const
{
  return mHeight;
}

uint32_t VideoEncoder::vscale() const
{
  if ( mWidth / 160 != mHeight / 102 )
    throw Ex{} << "Scale error";

  return mWidth / 160;
}

int VideoEncoder::pushFrame( AVCodecContext *c, AVStream *st, AVFrame const* frame )
{
  int ret;

  // send the frame to the encoder
  ret = avcodec_send_frame( c, frame );
  if ( ret < 0 )
  {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror( ret, errbuf, AV_ERROR_MAX_STRING_SIZE );
    throw Ex{} << "Error sending a frame to the encoder: " << errbuf;
  }

  while ( ret >= 0 )
  {
    AVPacket pkt{};

    ret = avcodec_receive_packet( c, &pkt );
    if ( ret == AVERROR( EAGAIN ) || ret == AVERROR_EOF )
      break;
    else if ( ret < 0 )
    {
      char errbuf[AV_ERROR_MAX_STRING_SIZE];
      av_strerror( ret, errbuf, AV_ERROR_MAX_STRING_SIZE );
      throw Ex{} << "Error encoding a frame: " << errbuf;
    }

    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts( &pkt, c->time_base, st->time_base );
    pkt.stream_index = st->index;

    /* Write the compressed frame to the media file. */
    ret = av_interleaved_write_frame( mFormatContext, &pkt );
    av_packet_unref( &pkt );
    if ( ret < 0 )
    {
      char errbuf[AV_ERROR_MAX_STRING_SIZE];
      av_strerror( ret, errbuf, AV_ERROR_MAX_STRING_SIZE );
      throw Ex{} << "Error while writing output packet: " << errbuf;
    }
  }

  return ret == AVERROR_EOF ? 1 : 0;
}

/**************************************************************/
/* audio output */

std::shared_ptr<AVFrame> VideoEncoder::allocAudioFrame( enum AVSampleFormat sample_fmt, uint64_t channel_layout, int sample_rate, int nb_samples )
{
  std::shared_ptr<AVFrame> frame = std::shared_ptr<AVFrame>( av_frame_alloc(), []( AVFrame * f )
  {
    av_frame_free( &f );
  } );

  int ret;

  if ( !frame )
    throw Ex{} << "Error allocating an audio frame";

  frame->format = sample_fmt;
  frame->channel_layout = channel_layout;
  frame->sample_rate = sample_rate;
  frame->nb_samples = nb_samples;

  if ( nb_samples )
  {
    ret = av_frame_get_buffer( &*frame, 0 );
    if ( ret < 0 )
      throw Ex{} << "Error allocating an audio buffer";
  }

  return frame;
}

void VideoEncoder::openAudio( int abitrate )
{
  /* find the encoder */
  mAudioCodec = avcodec_find_encoder( mFormatContext->oformat->audio_codec );
  if ( !mAudioCodec )
  {
    throw Ex{} << "Could not find encoder for " << avcodec_get_name( mFormatContext->oformat->audio_codec );
  }

  mAudioStream = std::make_shared<StreamContext>();

  mAudioStream->st = avformat_new_stream( mFormatContext, NULL );
  if ( !mAudioStream->st )
  {
    throw Ex{} << "Could not allocate stream";
  }
  mAudioStream->st->id = mFormatContext->nb_streams - 1;
  mAudioStream->encoder = std::shared_ptr<AVCodecContext>( avcodec_alloc_context3( mAudioCodec ), []( AVCodecContext * c )
  {
    avcodec_free_context( &c );
  } );

  if ( !mAudioStream->encoder )
  {
    throw Ex{} << "Could not alloc an encoding context";
  }

  AVCodecContext & c = *mAudioStream->encoder;

  c.sample_fmt  = AV_SAMPLE_FMT_FLTP;
  c.bit_rate    = abitrate;
  c.sample_rate = SAMPLE_RATE;
  c.channel_layout = AV_CH_LAYOUT_STEREO;
  c.channels        = av_get_channel_layout_nb_channels( c.channel_layout );
  mAudioStream->st->time_base = AVRational{ 1, c.sample_rate };

  /* Some formats want stream headers to be separate. */
  if ( mFormatContext->oformat->flags & AVFMT_GLOBALHEADER )
    c.flags |= AV_CODEC_FLAG_GLOBAL_HEADER;


  int nb_samples;
  int ret;
  AVDictionary *opt = NULL;

  /* open it */
  ret = avcodec_open2( &*mAudioStream->encoder, mAudioCodec, &opt );
  if ( ret < 0 )
  {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror( ret, errbuf, AV_ERROR_MAX_STRING_SIZE );
    throw Ex{} << "Could not open audio codec:" << errbuf;
  }


  if ( mAudioStream->encoder->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE )
    nb_samples = 10000;
  else
    nb_samples = mAudioStream->encoder->frame_size;

  mAudioStream->frame = allocAudioFrame( mAudioStream->encoder->sample_fmt, mAudioStream->encoder->channel_layout, mAudioStream->encoder->sample_rate, nb_samples );

  /* copy the stream parameters to the muxer */
  ret = avcodec_parameters_from_context( mAudioStream->st->codecpar, &*mAudioStream->encoder );
  if ( ret < 0 )
    throw Ex{} << "Could not copy the stream parameters";
}

/**************************************************************/
/* video output */

std::shared_ptr<AVFrame> VideoEncoder::allocVideoFrame( enum AVPixelFormat pix_fmt, int width, int height )
{
  int ret;

  auto picture = std::shared_ptr<AVFrame>( av_frame_alloc(), []( AVFrame * f )
  {
    av_frame_free( &f );
  } );

  if ( !picture )
    throw Ex{} << "Could not allocate frame.";

  picture->format = pix_fmt;
  picture->width  = width;
  picture->height = height;

  /* allocate the buffers for the frame data */
  ret = av_frame_get_buffer( &*picture, 0 );
  if ( ret < 0 )
  {
    throw Ex{} << "Could not allocate frame data.";
  }

  return picture;
}

void VideoEncoder::openVideo( int width, int height, int bitrate, AVRational fps )
{
  /* find the encoder */
  mVideoCodec = avcodec_find_encoder( mFormatContext->oformat->video_codec );
  if ( !mVideoCodec )
  {
    throw Ex{} << "Could not find encoder for " << avcodec_get_name( mFormatContext->oformat->video_codec );
  }

  mVideoStream = std::make_shared<StreamContext>();

  mVideoStream->st = avformat_new_stream( mFormatContext, NULL );
  if ( !mVideoStream->st )
  {
    throw Ex{} << "Could not allocate stream";
  }
  mVideoStream->st->id = mFormatContext->nb_streams - 1;
  mVideoStream->encoder = std::shared_ptr<AVCodecContext>( avcodec_alloc_context3( mVideoCodec ), []( AVCodecContext * c )
  {
    avcodec_free_context( &c );
  } );

  if ( !mVideoStream->encoder )
  {
    throw Ex{} << "Could not alloc an encoding context";
  }

  AVCodecContext & c = *mVideoStream->encoder;

  c.codec_id = mFormatContext->oformat->video_codec;

  c.bit_rate = bitrate;
  /* Resolution must be a multiple of two. */
  c.width    = width;
  c.height   = height;
  /* timebase: This is the fundamental unit of time (in seconds) in terms
    * of which frame timestamps are represented. For fixed-fps content,
    * timebase should be 1/framerate and timestamp increments should be
    * identical to 1. */
  mVideoStream->st->time_base = fps;
  c.time_base       = mVideoStream->st->time_base;

  c.gop_size      = 12; /* emit one intra frame every twelve frames at most */
  c.pix_fmt       = AV_PIX_FMT_YUV420P;
  if ( c.codec_id == AV_CODEC_ID_MPEG2VIDEO )
  {
    /* just for testing, we also add B-frames */
    c.max_b_frames = 2;
  }
  if ( c.codec_id == AV_CODEC_ID_MPEG1VIDEO )
  {
    /* Needed to avoid using macroblocks in which some coeffs overflow.
      * This does not happen with normal video, it just happens here as
      * the motion of the chroma plane does not match the luma plane. */
    c.mb_decision = 2;
  }

  /* Some formats want stream headers to be separate. */
  if ( mFormatContext->oformat->flags & AVFMT_GLOBALHEADER )
    c.flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  int ret;
  AVDictionary *opt = NULL;

  /* open the codec */
  ret = avcodec_open2( &*mVideoStream->encoder, mVideoCodec, &opt );
  if ( ret < 0 )
  {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror( ret, errbuf, AV_ERROR_MAX_STRING_SIZE );
    throw Ex{} << "Could not open video codec: " << errbuf;
  }

  mVideoStream->frame = allocVideoFrame( mVideoStream->encoder->pix_fmt, mVideoStream->encoder->width, mVideoStream->encoder->height );

  /* If the output format is not YUV420P, then a temporary YUV420P
   * picture is needed too. It is then converted to the required
   * output format. */
  assert( mVideoStream->encoder->pix_fmt == AV_PIX_FMT_YUV420P );

  /* copy the stream parameters to the muxer */
  ret = avcodec_parameters_from_context( mVideoStream->st->codecpar, &*mVideoStream->encoder );
  if ( ret < 0 )
    throw Ex{} << "Could not copy the stream parameters";
}

/*
 * encode one video frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
bool VideoEncoder::writeFrame( uint8_t const* y, int ystride, uint8_t const* u, int ustride, uint8_t const* v, int vstride )
{
  if ( !mFormatContext )
    return false;

  /* when we pass a frame to the encoder, it may keep a reference to it
   * internally; make sure we do not overwrite it here */
  if ( av_frame_make_writable( &*mVideoStream->frame ) < 0 )
    throw Ex{};

  int height = mVideoStream->encoder->height;

  size_t ysize = mVideoStream->frame->linesize[0];
  for ( size_t i = 0; i < height; ++i )
  {
    memcpy( mVideoStream->frame->data[0] + i * ysize, y + i * ystride, ysize );
  }

  size_t usize = mVideoStream->frame->linesize[1];
  size_t vsize = mVideoStream->frame->linesize[2];
  for ( size_t i = 0; i < height / 2; ++i )
  {
    memcpy( mVideoStream->frame->data[1] + i * usize, u + i * ustride, usize );
    memcpy( mVideoStream->frame->data[2] + i * vsize, v + i * vstride, vsize );
  }

  mVideoStream->frame->pts = mVideoStream->nextPts++;

  pushFrame( &*mVideoStream->encoder, mVideoStream->st, &*mVideoStream->frame );

  while ( av_compare_ts( mVideoStream->nextPts, mVideoStream->encoder->time_base, mAudioStream->nextPts, mAudioStream->encoder->time_base ) > 0 )
  {
    int ret = av_frame_make_writable( &*mAudioStream->frame );
    if ( ret < 0 )
      throw Ex{};

    std::scoped_lock<std::mutex> l{ mMutex };

    if ( mAudioQueue.size() < mAudioStream->frame->nb_samples * 2 )
      return true;

    for ( int j = 0; j < mAudioStream->frame->nb_samples; j++ )
    {
      ( (float*)mAudioStream->frame->data[0] )[j] = mAudioQueue.front();
      mAudioQueue.pop();
      ( (float*)mAudioStream->frame->data[1] )[j] = mAudioQueue.front();
      mAudioQueue.pop();
    }

    mAudioStream->frame->pts = av_rescale_q( mAudioStream->nextPts, AVRational{ 1, mAudioStream->encoder->sample_rate }, mAudioStream->encoder->time_base );
    mAudioStream->nextPts  += mAudioStream->frame->nb_samples;

    pushFrame( &*mAudioStream->encoder, mAudioStream->st, &*mAudioStream->frame );
  }

  return true;
}

IEncoder* createEncoder( char const* path, int vbitrate, int abitrate, int width, int height )
{
  return new VideoEncoder( path, vbitrate, abitrate, width, height );
}

void disposeEncoder( IEncoder* encoder )
{
  if ( encoder )
    delete encoder;
}
