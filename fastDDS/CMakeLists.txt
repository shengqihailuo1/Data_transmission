cmake_minimum_required(VERSION 3.20)
project(my_hk_FastDDS)

if(NOT fastcdr_FOUND)
    find_package(fastcdr 2 REQUIRED)
endif()
if(NOT foonathan_memory_FOUND)
    find_package(foonathan_memory REQUIRED)
endif()
if(NOT fastrtps_FOUND)
    find_package(fastrtps 2.12 REQUIRED)
endif()
find_package(PkgConfig REQUIRED)
pkg_check_modules(YAML REQUIRED yaml-cpp)
find_package(OpenCV 4.2.0 REQUIRED)


#用于告知编译器去哪里寻找需要包含的头文件。
include_directories(
  ${YAML_INCLUDE_DIRS}
  ${OpenCV_INCLUDE_DIRS}   #OpenCV 库的头文件路径
  include                  #项目中的一个本地 include 目录
  /opt/MVS/include/        #安装在系统里面的海康SDK的头文件
)

#指定了编译器在链接阶段应该查找库文件的路径
LINK_DIRECTORIES(
  ${YAML_LIBRARY_DIRS}
  /opt/MVS/lib/64/
)

include(CheckCXXCompilerFlag)
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG OR
        CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    check_cxx_compiler_flag(-std=c++11 SUPPORTS_CXX11)
    if(SUPPORTS_CXX11)
        add_compile_options(-std=c++11)
    else()
        message(FATAL_ERROR "Compiler doesn't support C++11")
    endif()
endif()

#example方案
file(GLOB src_example_cxx "src/example/*.cxx")
add_executable(my_example_Publisher src/example/hk_image_Publisher.cpp ${src_example_cxx})
# target_link_libraries(my_hk_FastDDS fastdds fastrtps fastcdr)
target_link_libraries(my_example_Publisher
  fastrtps
  fastcdr
  ${YAML_LIBRARIES}
  #加载下面海康相机的.so文件并调用其API接口，实现对海康相机的控制和图像加载功能。
  MvCameraControl
  MVGigEVisionSDK
  MVRender
  MvUsb3vTL
  MediaProcess
  X11
)
add_executable(my_example_Subscriber src/example/hk_image_Subscriber.cpp ${src_example_cxx})
target_link_libraries(my_example_Subscriber fastrtps fastcdr ${OpenCV_LIBRARIES})



#共享内存方案
file(GLOB src_Shared_memory "src/Shared_memory/*.cxx")
add_executable(my_SharedMemory_Publisher src/Shared_memory/hk_image_Publisher.cpp ${src_Shared_memory})
target_link_libraries(my_SharedMemory_Publisher
  fastrtps
  fastcdr
  foonathan_memory
  ${YAML_LIBRARIES}
  #加载下面海康相机的.so文件并调用其API接口，实现对海康相机的控制和图像加载功能。
  MvCameraControl
  MVGigEVisionSDK
  MVRender
  MvUsb3vTL
  MediaProcess
  X11
)
add_executable(my_SharedMemory_Subscriber src/Shared_memory/hk_image_Subscriber.cpp ${src_Shared_memory})
target_link_libraries(my_SharedMemory_Subscriber fastrtps fastcdr ${OpenCV_LIBRARIES})


#DataSharing方案
file(GLOB src_DataSharing "src/DataSharing/*.cxx")
add_executable(my_DataSharing_Publisher src/DataSharing/hk_image_Publisher.cpp ${src_Shared_memory})
target_link_libraries(my_DataSharing_Publisher
  fastrtps
  fastcdr
  foonathan_memory
  ${YAML_LIBRARIES}
  #加载下面海康相机的.so文件并调用其API接口，实现对海康相机的控制和图像加载功能。
  MvCameraControl
  MVGigEVisionSDK
  MVRender
  MvUsb3vTL
  MediaProcess
  X11
)
add_executable(my_DataSharing_Subscriber src/DataSharing/hk_image_Subscriber.cpp ${src_Shared_memory})
target_link_libraries(my_DataSharing_Subscriber fastrtps fastcdr ${OpenCV_LIBRARIES})


#Zeor Copy 方案
file(GLOB src_ZeroCopy "src/Zero_Copy/*.cxx")
add_executable(my_ZeroCopy_Publisher src/Zero_Copy/hk_image_Publisher.cpp ${src_ZeroCopy})
target_link_libraries(my_ZeroCopy_Publisher
  fastrtps
  fastcdr
  foonathan_memory
  ${YAML_LIBRARIES}
  #加载下面海康相机的.so文件并调用其API接口，实现海康相机的控制和图像加载功能。
  MvCameraControl
  MVGigEVisionSDK
  MVRender
  MvUsb3vTL
  MediaProcess
  X11
)
add_executable(my_ZeroCopy_Subscriber src/Zero_Copy/hk_image_Subscriber.cpp ${src_ZeroCopy})
target_link_libraries(my_ZeroCopy_Subscriber fastrtps fastcdr ${OpenCV_LIBRARIES})


#TCP方案
# #将指定的文件复制到生成的构建目录中，保持文件的相对路径不变。
# #通常用于将必要的配置文件、证书文件或者其他资源文件复制到执行程序或者安装包所需的目录中，以便程序在运行时能够访问这些文件。
# configure_file("HelloWorldSubscriber_static_disc.xml" "HelloWorldSubscriber_static_disc.xml" COPYONLY)
# configure_file("HelloWorldPublisher_static_disc.xml" "HelloWorldPublisher_static_disc.xml" COPYONLY)
# configure_file("dh2048.pem" "dh2048.pem" COPYONLY)
# configure_file("serverkey.pem" "serverkey.pem" COPYONLY)
# configure_file("servercert.pem" "servercert.pem" COPYONLY)
# configure_file("cacert.pem" "cacert.pem" COPYONLY)
file(GLOB src_TCP "src/TCP/*.cxx")
add_executable(my_TCP_main src/TCP/my_TCP_main.cpp ${src_TCP} src/TCP/hk_image_Publisher.cpp src/TCP/hk_image_Subscriber.cpp)
target_link_libraries(my_TCP_main
  fastrtps
  fastcdr
  foonathan_memory
  fastdds::optionparser
  ${YAML_LIBRARIES}
  ${OpenCV_LIBRARIES}
  #加载下面海康相机的.so文件并调用其API接口，实现海康相机的控制和图像加载功能。
  MvCameraControl
  MVGigEVisionSDK
  MVRender
  MvUsb3vTL
  MediaProcess
  X11
)
# add_executable(my_TCP_Subscriber src/TCP/hk_image_Subscriber.cpp ${src_TCP})
# target_link_libraries(my_TCP_Subscriber fastrtps fastcdr ${OpenCV_LIBRARIES})
