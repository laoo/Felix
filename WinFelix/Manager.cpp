#include "pch.hpp"
#include "Manager.hpp"
#include "InputFile.hpp"
#include "WinRenderer.hpp"
#include "WinAudioOut.hpp"
#include "ComLynxWire.hpp"
#include "Core.hpp"
#include "imgui.h"

Manager::Manager() : mEmulationRunning{ true }, mHorizontalView{ true }, mDoUpdate{ false }, mIntputSources{}, mProcessThreads{ true }, mInstancesCount{ 1 }, mRenderThread{}, mAudioThread{}
{
  mIntputSources[0] = std::make_shared<InputSource>();
  mIntputSources[1] = std::make_shared<InputSource>();

  mRenderer = std::make_shared<WinRenderer>();
  mAudioOut = std::make_shared<WinAudioOut>();
  mComLynxWire = std::make_shared<ComLynxWire>();
}

void Manager::update()
{
  if ( mDoUpdate )
    reset();
  mDoUpdate = false;
}

void Manager::doArgs( std::vector<std::wstring> args )
{
  mArgs = std::move( args );
  reset();
}

void Manager::initialize( HWND hWnd )
{
  assert( mRenderer );
  mRenderer->initialize( hWnd );
}

int Manager::win32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
  assert( mRenderer );
  return mRenderer->win32_WndProcHandler( hWnd, msg, wParam, lParam );
}

Manager::~Manager()
{
  stopThreads();
}

void Manager::drawGui( int left, int top, int right, int bottom )
{
  ImGuiIO & io = ImGui::GetIO();

  bool hovered = io.MousePos.x > left && io.MousePos.y > top && io.MousePos.x < right && io.MousePos.y < bottom;

  if ( hovered )
  {
    ImGui::PushStyleVar( ImGuiStyleVar_Alpha, std::clamp( ( 100.0f - io.MousePos.y ) / 100.f, 0.0f, 1.0f ) );
    if ( ImGui::BeginMainMenuBar() )
    {
      ImGui::PushStyleVar( ImGuiStyleVar_Alpha, 1.0f );
      if ( ImGui::BeginMenu( "File" ) )
      {
        if ( ImGui::MenuItem( "Exit", "Alt+F4" ) )
        {
          mEmulationRunning = false;
        }
        ImGui::EndMenu();
      }
      if ( ImGui::BeginMenu( "View" ) )
      {
        ImGui::Checkbox( "Horizontal", &mHorizontalView );
        ImGui::EndMenu();
      }
      if ( ImGui::BeginMenu( "Options" ) )
      {
        bool doubleInstance = mInstancesCount > 1;
        if ( ImGui::Checkbox( "Double Instance", &doubleInstance ) )
        {
          mInstancesCount = doubleInstance ? 2 : 1;
          mDoUpdate = true;
        }
        
        ImGui::EndMenu();
      }
      ImGui::EndMainMenuBar();
      ImGui::PopStyleVar();
    }
    ImGui::PopStyleVar();
  }

  //ImGui::ShowDemoWindow();
}

void Manager::horizontalView( bool horizontal )
{
  mHorizontalView = horizontal;
}

bool Manager::doRun() const
{
  return mEmulationRunning;
}

bool Manager::horizontalView() const
{
  return mHorizontalView;
}

void Manager::processKeys()
{
  std::array<uint8_t, 256> keys;
  if ( !GetKeyboardState( keys.data() ) )
    return;

  mIntputSources[0]->left = keys['A'] & 0x80;
  mIntputSources[0]->up = keys['W'] & 0x80;
  mIntputSources[0]->right = keys['D'] & 0x80;
  mIntputSources[0]->down = keys['S'] & 0x80;
  mIntputSources[0]->opt1 = keys['1'] & 0x80;
  mIntputSources[0]->pause = keys['2'] & 0x80;
  mIntputSources[0]->opt2 = keys['3'] & 0x80;
  mIntputSources[0]->a = keys[VK_LCONTROL] & 0x80;
  mIntputSources[0]->b = keys[VK_LSHIFT] & 0x80;

  mIntputSources[1]->left = keys[VK_LEFT] & 0x80;
  mIntputSources[1]->up = keys[VK_UP] & 0x80;
  mIntputSources[1]->right = keys[VK_RIGHT] & 0x80;
  mIntputSources[1]->down = keys[VK_DOWN] & 0x80;
  mIntputSources[1]->opt1 = keys[VK_DELETE] & 0x80;
  mIntputSources[1]->pause = keys[VK_END] & 0x80;
  mIntputSources[1]->opt2 = keys[VK_NEXT] & 0x80;
  mIntputSources[1]->a = keys[VK_RCONTROL] & 0x80;
  mIntputSources[1]->b = keys[VK_RSHIFT] & 0x80;
}

std::shared_ptr<IInputSource> Manager::getInputSource( int instance )
{
  if ( instance >= 0 && instance < mIntputSources.size() )
    return mIntputSources[instance];
  else
    return {};
}

void Manager::reset()
{
  stopThreads();
  mProcessThreads.store( true );
  mInstances.clear();

  std::filesystem::path log{};
  std::vector<InputFile> inputs;

  for ( auto const& arg : mArgs )
  {
    std::filesystem::path path{ arg };
    if ( path.has_extension() && path.extension() == ".log" )
    {
      log = path;
      continue;
    }

    InputFile file{ path };
    if ( file.valid() )
    {
      inputs.push_back( file );
    }
  }

  if ( !inputs.empty() )
  {

    mRenderer->setInstances( mInstancesCount );

    for ( size_t i = 0; i < mInstancesCount; ++i )
    {
      mInstances.push_back( std::make_shared<Core>( mComLynxWire, mRenderer->getVideoSink( (int)i ), getInputSource( (int)i ), std::span<InputFile>{ inputs.data(), inputs.size() } ) );
      if ( !log.empty() )
        mInstances.back()->setLog( log );
    }

    mRenderThread = std::thread{ [this]
    {
      while ( mProcessThreads.load() )
      {
        mRenderer->render( *this );
      }
    } };

    mAudioThread = std::thread{ [this]
    {
      while ( mProcessThreads.load() )
      {
        mAudioOut->fillBuffer( std::span<std::shared_ptr<Core> const>{ mInstances.data(), mInstances.size() } );
      }
    } };
  }
}

void Manager::stopThreads()
{
  mProcessThreads.store( 0 );
  if ( mAudioThread.joinable() )
    mAudioThread.join();
  mAudioThread = {};
  if ( mRenderThread.joinable() )
    mRenderThread.join();
  mRenderThread = {};
}

Manager::InputSource::InputSource() : KeyInput{}
{
}

KeyInput Manager::InputSource::getInput() const
{
  return *this;
}
