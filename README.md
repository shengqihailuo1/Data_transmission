# 目录

- [ROS2](#ros2)
  * [install](#install)
  * [launch](#launch)
- [fastdds-无需依赖ROS2](#fastdds-无需依赖ROS2)
  * [共享内存传输](#共享内存传输)
  * [Data-sharing传输](#Data-sharing传输)
  * [TCP传输](#TCP传输)
  * [UDP传输](#UDP传输)
  * [Zero Copy传输](#Zero-Copy传输)
  * [总结](#总结)

# ROS2



## install

```_
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src
git clone https://github.com/shengqihailuo1/Data_transmission.git
cd ROS2
colcon build
source install/setup.bash
```

## launch

```
# NOTE：Before starting the node, you need to look up the Hikon camera serial number and replace "expect_serial_number" in my_camera.yaml

ros2 run HikRobot_Camera_ROS2 hk_camera --ros-args --params-file HikRobot_Camera_ROS2/config/my_camera.yaml
# or
ros2 launch HikRobot_Camera_ROS2 my_camera_launch.py
```





# fastdds-无需依赖ROS2

Fast DDS 支持多种数据传输方式，包括共享内存、Data-sharing、TCP/UDP、Zero Copy 等方法。在发布者应用程序中，通过SDK从相机设备获取图像数据，并通过 `DataWriter` 发布。在订阅者应用程序中，使用 `DataReader` 接收图像数据，并进行后续处理。

实现步骤：定义图像IDL数据结构、生成序列化和反序列化接口、编写数据发布和订阅程序、监控数据传输的性能指标。



## 共享内存传输

```
#通过fastdds_monitor监控数据传输的性能指标，参考：https://github.com/eProsima/Fast-DDS-monitor

cd ~/path/to/build
export FASTDDS_STATISTICS="HISTORY_LATENCY_TOPIC;NETWORK_LATENCY_TOPIC;\
PUBLICATION_THROUGHPUT_TOPIC;SUBSCRIPTION_THROUGHPUT_TOPIC;RTPS_SENT_TOPIC;\
RTPS_LOST_TOPIC;HEARTBEAT_COUNT_TOPIC;ACKNACK_COUNT_TOPIC;NACKFRAG_COUNT_TOPIC;\
GAP_COUNT_TOPIC;DATA_COUNT_TOPIC;RESENT_DATAS_TOPIC;SAMPLE_DATAS_TOPIC;\
PDP_PACKETS_TOPIC;EDP_PACKETS_TOPIC;DISCOVERY_TOPIC;PHYSICAL_DATA_TOPIC;\
MONITOR_SERVICE_TOPIC"
./my_hk_FastDDS_Publisher
 
#另一个终端窗口
cd ~/path/to/build
export FASTDDS_STATISTICS="HISTORY_LATENCY_TOPIC;NETWORK_LATENCY_TOPIC;\
PUBLICATION_THROUGHPUT_TOPIC;SUBSCRIPTION_THROUGHPUT_TOPIC;RTPS_SENT_TOPIC;\
RTPS_LOST_TOPIC;HEARTBEAT_COUNT_TOPIC;ACKNACK_COUNT_TOPIC;NACKFRAG_COUNT_TOPIC;\
GAP_COUNT_TOPIC;DATA_COUNT_TOPIC;RESENT_DATAS_TOPIC;SAMPLE_DATAS_TOPIC;\
PDP_PACKETS_TOPIC;EDP_PACKETS_TOPIC;DISCOVERY_TOPIC;PHYSICAL_DATA_TOPIC;\
MONITOR_SERVICE_TOPIC"
./my_hk_FastDDS_Subscriber

##另一个终端窗口，打开fastdds_monitor监控程序，测量通信参数（延迟、吞吐量、丢包等），并实时记录和计算这些参数的统计测量结果（均值、方差、标准差等）。 
cd /usr/local/bin && ./fastdds_monitor
```

![image-20240727195542779](./assets/image-20240727195542779.png)

左上角图：发布者和订阅者之间的延迟时间（显示最近一分钟，单位：us）

右上角图：发布者和订阅者之间的延迟时间（显示历史数据，单位：us）

左下角图：发布者的吞吐量（单位：B/S）

右下角图：订阅者的吞吐量（单位：B/S）



## Data-sharing传输

<img src="./assets/image-20240728110521342.png" alt="image-20240728110521342" style="zoom:50%;" />

左上角图：发布者和订阅者之间的延迟时间（显示最近一分钟，单位：us）

右上角图：发布者和订阅者之间的延迟时间（显示历史数据，单位：us）

左下角图：发布者的吞吐量（单位：B/S）

右下角图：订阅者的吞吐量（单位：B/S）



## TCP传输

![image-20240731170052429](./assets/image-20240731170052429.png)

左上角图：发布者和订阅者之间的延迟时间（显示最近一分钟，单位：us）

右上角图：发布者和订阅者之间的延迟时间（显示历史数据，单位：us）

左下角图：发布者的吞吐量（单位：B/S）

右下角图：订阅者的吞吐量（单位：B/S）



## UDP传输

![image-20240725162115949](./assets/image-20240725162115949.png)

左上角图：发布者和订阅者之间的延迟时间（显示最近一分钟，单位：us）

右上角图：发布者和订阅者之间的延迟时间（显示历史数据，单位：us）

左下角图：发布者的吞吐量（单位：B/S）

右下角图：订阅者的吞吐量（单位：B/S）



## Zero Copy传输

![image-20240729183444674](./assets/image-20240729183444674.png)

左上角图：发布者和订阅者之间的延迟时间（显示最近一分钟，单位：us）

右上角图：发布者和订阅者之间的延迟时间（显示历史数据，单位：us）

左下角图：发布者的吞吐量（单位：B/S）

右下角图：订阅者的吞吐量（单位：B/S）



## 总结

传输数据：图像帧率50HZ，单张图像数据量 = 1280 * 1024 * 3

指标定义：在 DDS 通信交换的情况下，“延迟时间”为 DataWriter 序列化和发送数据消息所需的时间加上匹配的 DataReader 接收和反序列化它所需的时间。 

结论：在50HZ、吞吐量193MB/S的情况下，Zero Copy零拷贝方案的延迟时间仅为87us。

|                  | 延迟时间（us） | 吞吐量（B/S） |
| ---------------- | -------------- | ------------- |
| **共享内存**     | 1809.773       | 193311792     |
| **Data-sharing** | 1177.878       | 193123904     |
| **TCP**          | 1584.2654      | 193255744     |
| **UDP**          | 1027.426       | 193245861     |
| **Zero Copy**    | 87.0395        | 192995200     |









