cmake_minimum_required(VERSION 2.8)
project(RtspServer)
set(LIVE_DIR_PRE "${PROJECT_SOURCE_DIR}/live")
set(LIVE_LIBS "${PROJECT_SOURCE_DIR}/live/lib/")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -fpermissive -g")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -O2 -g")
include_directories(${LIVE_DIR_PRE}/include/groupsock/include)
include_directories(${LIVE_DIR_PRE}/include/BasicUsageEnvironment/include)
include_directories(${LIVE_DIR_PRE}/include/liveMedia/include)
include_directories(${LIVE_DIR_PRE}/include/UsageEnvironment/include)
link_directories(${LIVE_LIBS})

add_executable(RtspServer main.cpp cJSON.c  DynamicRTSPServer.cpp H264FramedLiveSource.cpp itcRtspClient.cpp
     H265LiveVideoServerMediaSubssion.cpp Workthread.cpp )
target_link_libraries(RtspServer rt m pthread liveMedia BasicUsageEnvironment groupsock  UsageEnvironment)

