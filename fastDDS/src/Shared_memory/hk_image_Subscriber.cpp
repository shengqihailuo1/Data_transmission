#include "hk_image_Subscriber.h"

#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/rtps/transport/shared_mem/SharedMemTransportDescriptor.h>
#include <fastdds/rtps/transport/UDPv4TransportDescriptor.h>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;
using namespace eprosima::fastrtps::rtps;

hk_image_Subscriber::hk_image_Subscriber()
    : participant_(nullptr)
    , subscriber_(nullptr)
    , topic_(nullptr)
    , reader_(nullptr)
    , type_(new hk_imagePubSubType())
{
}

bool hk_image_Subscriber::init()
{
    //创建 PARTICIPANT （和publisher一样）
    DomainParticipantQos pqos;

    pqos.wire_protocol().builtin.discovery_config.discoveryProtocol = DiscoveryProtocol_t::SIMPLE;
    pqos.wire_protocol().builtin.discovery_config.use_SIMPLE_EndpointDiscoveryProtocol = true;
    pqos.wire_protocol().builtin.discovery_config.m_simpleEDP.use_PublicationReaderANDSubscriptionWriter = true;
    pqos.wire_protocol().builtin.discovery_config.m_simpleEDP.use_PublicationWriterANDSubscriptionReader = true;
    pqos.wire_protocol().builtin.discovery_config.leaseDuration = eprosima::fastrtps::c_TimeInfinite;

    pqos.name("Participant_sub");

/*
//下面4行配置为共享内存方式，会导致monirot看不到话题信息(注释掉就可以看到，但这样就没有共享内存方式了)。。。

    // 共享内存传输的显式配置
    pqos.transport().use_builtin_transports = false;
    //创建一个共享内存传输描述符
    auto sm_transport = std::make_shared<SharedMemTransportDescriptor>();
    // 设置其段的大小为一张图像大小的3倍,为11.25MB
    sm_transport->segment_size((1280*1024*3)*3);
    //将其添加到pqos的用户传输列表中。
    pqos.transport().user_transports.push_back(sm_transport);
*/

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pqos);

    if (participant_ == nullptr)
    {
        return false;
    }

    type_.register_type(participant_);

    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);

    if (subscriber_ == nullptr)
    {
        return false;
    }

    topic_ = participant_->create_topic("hk_image_SharedMemTopic", "hk_image", TOPIC_QOS_DEFAULT);

    if (topic_ == nullptr)
    {
        return false;
    }


    //配置DATAREADER数据读取者的历史记录、资源限制、可靠性和持久性
    DataReaderQos rqos;
    //（1）历史记录：保留最后30条数据样本。
    rqos.history().kind = KEEP_LAST_HISTORY_QOS;
    rqos.history().depth = 30;
    //（2）资源限制：最多能处理50个数据样本，初始时已分配20个样本的资源。
    rqos.resource_limits().max_samples = 50;
    rqos.resource_limits().allocated_samples = 20;
    //（3）可靠性：要求数据传输的高可靠性。
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    //（4）持久性：希望数据在本地缓存中短暂保存，以便在之后能够访问到。
    rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;

    reader_ = subscriber_->create_datareader(topic_, rqos, &listener_);


//下面是我改的，用于测试monitor。。。
//reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, &listener_);


    if (reader_ == nullptr)
    {
        return false;
    }

    return true;
}

hk_image_Subscriber::~hk_image_Subscriber()
{
    if (reader_ != nullptr)
    {
        subscriber_->delete_datareader(reader_);
    }
    if (topic_ != nullptr)
    {
        participant_->delete_topic(topic_);
    }
    if (subscriber_ != nullptr)
    {
        participant_->delete_subscriber(subscriber_);
    }
    DomainParticipantFactory::get_instance()->delete_participant(participant_);
}

void hk_image_Subscriber::SubListener::on_subscription_matched(
        DataReader*,
        const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        matched_ = info.total_count;
        std::cout << "Subscriber matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        matched_ = info.total_count;
        std::cout << "Subscriber unmatched." << std::endl;
    }
    else
    {
        std::cout << info.current_count_change
                  << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
    }
}

void hk_image_Subscriber::SubListener::on_data_available(
        DataReader* reader)
{
    SampleInfo info;
    if (reader->take_next_sample(&hk_image_, &info) == ReturnCode_t::RETCODE_OK)
    {
        if (info.valid_data)
        {
            //在这里处理接收到的数据。。。
            std::cout << "接收到: "<<" index: " << hk_image_.index() <<" message: " << hk_image_.message() <<" width: "<<hk_image_.width()<<" height: "<<hk_image_.height()<< " timestamp: "<<hk_image_.timestamp()<< std::endl;

            // cv::Mat image(1024, 1280, CV_8UC3, const_cast<uint8_t*>(hk_image_.image_data().data()));
            // cv::imshow("Image", image);
            // cv::waitKey(10);
        }
    }
}

int main(int argc, char** argv)
{
    std::cout << "Starting subscriber hk_image..." << std::endl;

    hk_image_Subscriber* my_sub = new hk_image_Subscriber();
    if(my_sub->init())
    {
        std::cout<<"init() 完成！开始接收消息..."<<std::endl;
        while(1){
            std::this_thread::sleep_for(std::chrono::seconds(5));//主函数休眠，在mysub里面接收到消息后调用回调函数on_data_available（）。这里处理的不是很好。。。
        }
    }

    delete my_sub;
    std::cout<<"退出程序..."<<std::endl;
    return 0;
}