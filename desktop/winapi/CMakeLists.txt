cmake_minimum_required(VERSION 3.10.0)
project(desktopclient CXX)
include_directories(".")
add_definitions(-DUNICODE -DNOMINMAX -DCURL_STATICLIB)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
set(CMAKE_CXX_FLAGS_DEBUG  "-g")

file(GLOB headers "*.h")
file(GLOB sources "*.cpp")

add_executable(desktopclient ${sources} ${headers} resources/res.rc)
target_link_libraries(desktopclient kernel32 user32 gdi32 shlwapi comctl32 curl ssh2 gcrypt gpg-error intl gnutls hogweed nettle idn2 atomic crypt32 gmp unistring iconv charset wldap32 z ws2_32 pthread)
set_target_properties(desktopclient PROPERTIES LINKER_LANGUAGE CXX)

add_custom_target(Headers SOURCES ${headers})
add_custom_target(Sources SOURCES ${sources})
