#pragma once

#include <array>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <span>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <boost/rational.hpp>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <Shlobj.h>
#include <atlbase.h>
#include <AudioClient.h>
#include <MMDeviceAPI.h>
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
