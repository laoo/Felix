#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <span>
#include <string>
#include <vector>
#include <cwchar>

#include <boost/rational.hpp>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <Shlobj.h>
#pragma warning(push)
#pragma warning( disable: 4005 )
#include <d3d11.h>
#pragma warning(pop)
#include <d3dcompiler.h>

#include <wrl/client.h>
template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#endif

#include "sol/sol.hpp"


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
