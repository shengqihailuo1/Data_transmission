cmake_minimum_required(VERSION 3.5)
project(HikRobot_Camera_ROS2)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

#启用编译优化选项通常会导致编译时间稍微增加，但是会显著提升程序的运行速度。这对于需要高性能的应用程序（如科学计算、图形处理、大规模数据处理等）特别有用
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(image_transport REQUIRED)
find_package(OpenCV 4.2.0 REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(camera_info_manager REQUIRED)

#用于告知编译器去哪里寻找需要包含的头文件。
include_directories(
  ${OpenCV_INCLUDE_DIRS}   #OpenCV 库的头文件路径
  include                  #项目中的一个本地 include 目录
  /opt/MVS/include/        #安装在系统里面的海康SDK的头文件
)

#指定了编译器在链接阶段应该查找库文件的路径
LINK_DIRECTORIES(
/opt/MVS/lib/64/
)

add_executable(hk_camera src/hk_camera.cpp)
ament_target_dependencies(hk_camera
  rclcpp
  std_msgs
  cv_bridge
  image_transport
)
target_link_libraries(hk_camera
  ${OpenCV_LIBRARIES}
  #加载下面海康相机的.so文件并调用其API接口，实现对海康相机的控制和图像加载功能。
  MvCameraControl
  MVGigEVisionSDK
  MVRender
  MvUsb3vTL
  MediaProcess
  X11
)

install(TARGETS
  hk_camera  
  DESTINATION lib/${PROJECT_NAME}#指定了安装的目标路径
)
# 安装launch文件
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})#指定了安装的目标路径

# 在install目录下安装参数配置文件.yaml
install(DIRECTORY config/
DESTINATION share/${PROJECT_NAME}/config
FILES_MATCHING PATTERN "*.yaml"
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
