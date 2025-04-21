#include "hk_image_Publisher.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/rtps/transport/TCPv4TransportDescriptor.h>
#include <fastrtps/utils/IPLocator.h>

#include <thread>
#include "../../include/hk_device.hpp"


using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;

hk_image_Publisher::hk_image_Publisher()
    : participant_(nullptr)
    , publisher_(nullptr)
    , topic_(nullptr)
    , writer_(nullptr)
    , type_(new hk_imagePubSubType())
{
}

bool hk_image_Publisher::init(
        const std::string& wan_ip,
        unsigned short port,
        bool use_tls,
        const std::vector<std::string>& whitelist)
{
    hk_image_.index(0);
    hk_image_.message("hk_image");

    //CREATE THE PARTICIPANT
    DomainParticipantQos pqos;
    pqos.wire_protocol().builtin.discovery_config.leaseDuration = eprosima::fastrtps::c_TimeInfinite;
    pqos.wire_protocol().builtin.discovery_config.leaseDuration_announcementperiod =
            eprosima::fastrtps::Duration_t(5, 0);
    pqos.name("Participant_pub");

    // pqos.transport().use_builtin_transports = false;//置为false将禁用默认的UDPv4实现。注意：和共享内存方案一样，导致monitor没有话题。
    pqos.transport().use_builtin_transports = true;//我手动改为true，不知道是否有影响？

    std::shared_ptr<TCPv4TransportDescriptor> descriptor = std::make_shared<TCPv4TransportDescriptor>();

    for (std::string ip : whitelist)
    {
        descriptor->interfaceWhiteList.push_back(ip);
        std::cout << "Whitelisted " << ip << std::endl;
    }

    if (use_tls)
    {
        using TLSOptions = TCPTransportDescriptor::TLSConfig::TLSOptions;
        descriptor->apply_security = true;
        descriptor->tls_config.password = "test";
        descriptor->tls_config.cert_chain_file = "servercert.pem";
        descriptor->tls_config.private_key_file = "serverkey.pem";
        descriptor->tls_config.tmp_dh_file = "dh2048.pem";
        descriptor->tls_config.add_option(TLSOptions::DEFAULT_WORKAROUNDS);
        descriptor->tls_config.add_option(TLSOptions::SINGLE_DH_USE);
        descriptor->tls_config.add_option(TLSOptions::NO_SSLV2);
    }

    descriptor->sendBufferSize = 0;
    descriptor->receiveBufferSize = 0;

    if (!wan_ip.empty())
    {
        descriptor->set_WAN_address(wan_ip);
        std::cout << wan_ip << ":" << port << std::endl;
    }
    descriptor->add_listener_port(port);
    pqos.transport().user_transports.push_back(descriptor);

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pqos);
    if (participant_ == nullptr)
    {
        return false;
    }

    //REGISTER THE TYPE
    type_.register_type(participant_);

    //CREATE THE PUBLISHER
    publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT);
    if (publisher_ == nullptr)
    {
        return false;
    }

    //CREATE THE TOPIC
    topic_ = participant_->create_topic("hk_image_TCP_topic", "hk_image", TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr)
    {
        return false;
    }

    //CREATE THE DATAWRITER
    DataWriterQos wqos;
    wqos.history().kind = KEEP_LAST_HISTORY_QOS;
    wqos.history().depth = 30;
    wqos.resource_limits().max_samples = 50;
    wqos.resource_limits().allocated_samples = 20;
    wqos.reliable_writer_qos().times.heartbeatPeriod.seconds = 2;
    wqos.reliable_writer_qos().times.heartbeatPeriod.nanosec = 200 * 1000 * 1000;
    wqos.reliability().kind = RELIABLE_RELIABILITY_QOS;

    writer_ = publisher_->create_datawriter(topic_, wqos, &listener_);
    if (writer_ == nullptr)
    {
        return false;
    }

    return true;
}

hk_image_Publisher::~hk_image_Publisher()
{
    if (writer_ != nullptr)
    {
        publisher_->delete_datawriter(writer_);
    }
    if (publisher_ != nullptr)
    {
        participant_->delete_publisher(publisher_);
    }
    if (topic_ != nullptr)
    {
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
        std::cout << "[RTCP] Publisher matched" << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        matched_ = info.total_count;
        std::cout << "[RTCP] Publisher unmatched" << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for PublicationMatchedStatus current count change" << std::endl;
    }
}

bool hk_image_Publisher::start_publish()
{
    camera::Camera MVS_cap;//相机设备 //注意：这个定义应该放在.h文件的private中。

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

            writer_->write((void*)&hk_image_);
            // writer_->write(&hk_image_);//把数据发送出去。
            std::cout<<"发送成功"<<std::endl;
        }else{
            std::cout << "等待订阅者..." << std::endl;
        }
    }
}
