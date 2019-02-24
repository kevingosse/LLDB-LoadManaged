cmake_minimum_required(VERSION 3.13)
project(testplugin)

set(CMAKE_CXX_STANDARD 14)

include_directories(~/lldb/include)


add_library(loadmanaged SHARED library.cpp library.h coreclrhost.h coreruncommon.cpp coreruncommon.h services.h pal_mstypes.h mstypes.h lldbservices.h unknwn.h services.cpp sosplugin.h ClrInterop.cpp)

target_link_libraries(loadmanaged ${CMAKE_DL_LIBS})