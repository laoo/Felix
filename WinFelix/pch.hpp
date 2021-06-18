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

#include <boost/rational.hpp>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#pragma warning(push)
#pragma warning( disable: 4005 )
#include <d3d11.h>
#pragma warning(pop)
#include <d3dcompiler.h>

#include <wrl/client.h>
template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;

#endif
