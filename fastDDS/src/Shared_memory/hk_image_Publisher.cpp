#include "hk_image_Publisher.h"

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.h>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>

#include <thread>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;
using namespace eprosima::fastrtps::rtps;

hk_image_Publisher::hk_image_Publisher()
    : participant_(nullptr)
    , publisher_(nullptr)
    , topic_(nullptr)
    , writer_(nullptr)
    , type_(new hk_imagePubSubType())
{
}

bool hk_image_Publisher::init()
{
    // hk_image_ = std::make_shared<hk_image>();
    hk_image_.index(0);
    hk_image_.message("hk_image");

    //创建一个DomainParticipant
    DomainParticipantQos pqos;

    //使用简单的发现协议
    pqos.wire_protocol().builtin.discovery_config.discoveryProtocol = DiscoveryProtocol_t::SIMPLE;
    //启用内置的发布者和订阅者
    pqos.wire_protocol().builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = true;
    pqos.wire_protocol().builtin.discovery_config.m_simpleEDP.use_PublicationReaderANDSubscriptionWriter = true;
    pqos.wire_protocol().builtin.discovery_config.m_simpleEDP.use_PublicationWriterANDSubscriptionReader = true;
    //设置时间为无限
    pqos.wire_protocol().builtin.discovery_config.leaseDuration = eprosima::fastrtps::c_TimeInfinite;

    //为参与者指定名称
    pqos.name("Participant_pub");

/*
//下面4行配置为共享内存方式，会导致monirot看不到话题信息(注释掉就可以看到，但这样就没有共享内存方式了)。。。

    // 共享内存传输的显式配置，设置为false将禁用默认的UDPv4实现（默认为true）【必须，这样通信才能完全使用共享内存执行。】
    pqos.transport().use_builtin_transports = false;
    //创建一个共享内存传输描述符
    std::shared_ptr<SharedMemTransportDescriptor>  shm_transport = std::make_shared<SharedMemTransportDescriptor>();
    // 设置其段的大小为一张图像大小的3倍,为11.25MB
    shm_transport->segment_size((1280*1024*3)*3);

    // // [OPTIONAL] ThreadSettings configuration
    // shm_transport->default_reception_threads(eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1});
    // shm_transport->set_thread_config_for_port(12345, eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1});
    // shm_transport->dump_thread(eprosima::fastdds::rtps::ThreadSettings{-1, 0, 0, -1});

    //将“共享内存”添加到pqos的用户传输列表中。后面用pqos配置participant。（相当于添加使用这个“共享内存”传输模式）
    pqos.transport().user_transports.push_back(shm_transport);
*/


    //使用配置好的pqos创建一个参与者
    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pqos);
    if (participant_ == nullptr)
    {
        return false;
    }

    //注册hk_image类型
    type_.register_type(participant_);

    //创建一个默认配置的发布者
    publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
    if (publisher_ == nullptr)
    {
        return false;
    }

    //创建话题
    topic_ = participant_->create_topic("hk_image_SharedMemTopic", "hk_image", TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr)
    {
        return false;
    }


    //配置DATAWRITER的历史记录、资源限制、可靠性和发布模式
    DataWriterQos wqos;
    wqos.history().kind = KEEP_LAST_HISTORY_QOS;
    wqos.history().depth = 30;
    wqos.resource_limits().max_samples = 50;
    wqos.resource_limits().allocated_samples = 20;
    wqos.reliable_writer_qos().times.heartbeatPeriod.seconds = 2;
    wqos.reliable_writer_qos().times.heartbeatPeriod.nanosec = 200 * 1000 * 1000;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    wqos.publish_mode().kind = ASYNCHRONOUS_PUBLISH_MODE;


    writer_ = publisher_->create_datawriter(topic_, wqos, &listener_);

//下面是我改的。用于测试monitor。。。
//writer_ = publisher_->create_datawriter(topic_,DATAWRITER_QOS_DEFAULT,&listener_);
    if (writer_ == nullptr)
    {
        return false;
    }

    return true;
}

hk_image_Publisher::~hk_image_Publisher()
{
//participant_里面是topic和publisher_，publisher_里面是writer_
    if (writer_ != nullptr){
        publisher_->delete_datawriter(writer_);
    }
    if(publisher_ != nullptr){
        participant_->delete_publisher(publisher_);
    }
    if(topic_ != nullptr){
        participant_->delete_topic(topic_);
    }
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
}

void hk_image_Publisher::PubListener::on_publication_matched(
        DataWriter*,
        const PublicationMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched_ = info.total_count;
        std::cout << "Publisher matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        matched_ = info.total_count;
        std::cout << "Publisher unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for PublicationMatchedStatus current count change" << std::endl;
    }
}

bool hk_image_Publisher::start_publish()
{
    int image_empty_count = 0;
    int nRet;
    while(true)
    {
        // 从相机中获取一帧图像，存放到pData里，图像信息放在stImageInfo，超时时间20毫秒
        nRet = MV_CC_GetOneFrameTimeout(MVS_cap.get_handle(), MVS_cap.get_pData(), MVS_cap.get_nDataSize(), MVS_cap.get_stImageInfo(), 20);
        if (nRet != MV_OK){
            std::cout<<"MV_CC_GetOneFrameTimeout failed number: "<<image_empty_count<<std::endl;
            if (++image_empty_count > 100){
                std::cout<<"The Number of Faild Reading Exceed The Set Value: "<<image_empty_count<<std::endl;
                exit(-1);//连续100张空图就退出
            }
            continue;
        }
        image_empty_count = 0; 
        // 从设备获取的源数据    
        MVS_cap.get_stConvertParam()->nWidth = MVS_cap.get_stImageInfo()->nWidth;              // 图像宽 
        MVS_cap.get_stConvertParam()->nHeight = MVS_cap.get_stImageInfo()->nHeight;            // 图像高 
        MVS_cap.get_stConvertParam()->pSrcData = MVS_cap.get_pData();                          // 输入数据缓存  
        MVS_cap.get_stConvertParam()->nSrcDataLen = MVS_cap.get_stImageInfo()->nFrameLen;      // 输入数据大小 
        MVS_cap.get_stConvertParam()->enSrcPixelType = MVS_cap.get_stImageInfo()->enPixelType; // 输入数据像素格式 
        // 转换像素格式后的目标数据
        MVS_cap.get_stConvertParam()->enDstPixelType = PixelType_Gvsp_BGR8_Packed;             // 输出像素格式 
        MVS_cap.get_stConvertParam()->pDstBuffer = MVS_cap.get_p_DataForRGB();                 // 输出数据缓存 
        MVS_cap.get_stConvertParam()->nDstBufferSize = 3 * MVS_cap.get_stImageInfo()->nHeight * MVS_cap.get_stImageInfo()->nWidth; // 缓存大小（输入？）
        //将采集到的原始数据转换为像素格式并保存到指定的内存中。
        nRet = MV_CC_ConvertPixelType(MVS_cap.get_handle(), MVS_cap.get_stConvertParam());
        if (MV_OK != nRet){
            printf("-----ERROR: MV_CC_ConvertPixelType failed! , [%x] \n", nRet);//报错：nRet==0x80000000：错误或无效句柄，重启！
        }

        if(listener_.matched_ > 0 ){//当前正在监听我的节点的数量>0(有人在听我，我才发送数据。不然就不发送，节省能量)
            hk_image_.index(hk_image_.index() + 1);
            hk_image_.message("message");
            hk_image_.height(MVS_cap.get_stImageInfo()->nHeight);
            hk_image_.width(MVS_cap.get_stImageInfo()->nWidth);
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            uint64_t current_time_milliseconds = static_cast<uint64_t>(milliseconds);
            hk_image_.timestamp(current_time_milliseconds);


            //图像数据：MVS_cap.get_p_DataForRGB()，它是指针（unsigned char *，指向大小为height * width * 3的内存），hk_image::image_data要求输入是“std::array<uint8_t, 3*1280*1024>&”类型。
            //图像大小：3 * MVS_cap.get_stImageInfo()->nHeight * MVS_cap.get_stImageInfo()->nWidth
            
            /*
                方案一：设置.idl文件中的传递图像数据的变量类型为std::array<uint8_t,3*1280*1024>
                由于 std::array 是一个固定大小的容器，不能直接将指针（MVS_cap.get_p_DataForRGB()）赋值给它。需要将指针指向的数据复制（std::copy）到 std::array 中。
            */
            // // 计算图像数据大小
            // size_t image_size = 3 * MVS_cap.get_stImageInfo()->nHeight * MVS_cap.get_stImageInfo()->nWidth;
            // // 确保图像大小不超过 std::array 的容量
            // constexpr size_t array_capacity = 3 * 1280 * 1024;
            // if (image_size != array_capacity) {
            //     std::cerr << "Error: 图像数据大小和std::array大小不匹配,无法复制." << std::endl;
            //     exit(-1);
            // }
            std::array<uint8_t, 3*1280*1024> temp_array;
            std::copy(MVS_cap.get_p_DataForRGB(), MVS_cap.get_p_DataForRGB() + (3 * MVS_cap.get_stImageInfo()->nHeight * MVS_cap.get_stImageInfo()->nWidth), temp_array.begin());
            hk_image_.image_data(temp_array);

            /*
                方案二：设置.idl文件中的传递图像数据的变量类型为std::vector<uint8_t>
            */
            // std::vector<uint8_t> temp_Vector(MVS_cap.get_p_DataForRGB(), MVS_cap.get_p_DataForRGB() + 3 * MVS_cap.get_stImageInfo()->nHeight * MVS_cap.get_stImageInfo()->nWidth);
            // hk_image_.image_data(temp_Vector);

            writer_->write(&hk_image_);//把数据发送出去。
            std::cout<<"发送成功"<<std::endl;
        }else{
            std::cout << "等待订阅者..." << std::endl;
        }
    }
}

int main(){
    hk_image_Publisher* my_pub = new hk_image_Publisher();
    if(my_pub->init()){
        std::cout<<"init() 完成！开始发布..."<<std::endl;
        my_pub->start_publish();
    }

    delete my_pub;

    return 0;
}

//问题1：monitor无法监控到共享内存方案的节点，但是可以监控到example方案的节点。(用官方HelloWorldExampleSharedMem代码，也无法在monitor看到节点)
//发现：把hk_image_Publisher.cpp的初始化init（）配置改为和example一样，就可以monitor到节点了。但是！！！这样就不是共享内存模式了！
//对比看看这些配置哪里有问题。。。

//问题2：无法用两份代码同时打开一个设备：MV_CC_OpenDevice fail! nRet [80000203 没有许可]。打开设备->获取图像->分给两份代码。