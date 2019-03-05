cmake_minimum_required(VERSION 3.13)
project(testplugin)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(~/llvm-project/lldb/include)

add_library(loadmanaged SHARED library.cpp library.h coreclrhost.h coreruncommon.cpp coreruncommon.h services.h pal_mstypes.h mstypes.h lldbservices.h unknwn.h services.cpp sosplugin.h ClrInterop.cpp)

target_link_libraries(loadmanaged ${CMAKE_DL_LIBS})