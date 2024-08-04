
set( Felix_VERSION_MAJOR 0 )
set( Felix_VERSION_MINOR 0 )
set( Felix_VERSION_PATCH 0 )
set( Felix_VERSION_TWEAK 0 )
set( Felix_VERSION_HASH "" )

find_package(Git)
if(Git_FOUND)

  execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --long
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE result
    OUTPUT_VARIABLE Felix_GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if(result)
    message(WARNING "Failed to get hash of last change: ${result}")
  else()
    string( REGEX REPLACE "^v([0-9]+)\\..*" "\\1" Felix_VERSION_MAJOR "${Felix_GIT_DESCRIBE}" )
    string( REGEX REPLACE "^v[0-9]+\\.([0-9]+).*" "\\1" Felix_VERSION_MINOR "${Felix_GIT_DESCRIBE}" )
    string( REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.([0-9]+).*" "\\1" Felix_VERSION_PATCH "${Felix_GIT_DESCRIBE}" )
    string( REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+\-([0-9]+).*" "\\1" Felix_VERSION_TWEAK "${Felix_GIT_DESCRIBE}" )
    string( REGEX REPLACE "^v[0-9]+\\.[0-9]+\\.[0-9]+\-[0-9]+\-g(.*)" "\\1" Felix_VERSION_HASH "${Felix_GIT_DESCRIBE}" )
  endif()
endif()

set( Felix_VERSION "${Felix_VERSION_MAJOR}.${Felix_VERSION_MINOR}.${Felix_VERSION_PATCH}" )

if ( Felix_VERSION_TWEAK EQUAL 0 )
  set( Felix_VERSION_LONG ${Felix_VERSION} )
else()
  set( Felix_VERSION_LONG "${Felix_VERSION}.${Felix_VERSION_TWEAK} (${Felix_VERSION_HASH})" )
endif()
