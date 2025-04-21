#include "hk_image_Subscriber.h"

#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/rtps/transport/TCPv4TransportDescriptor.h>
#include <fastrtps/utils/IPLocator.h>

using namespace eprosima::fastdds::dds;
using namespace eprosima::fastdds::rtps;

using IPLocator = eprosima::fastrtps::rtps::IPLocator;

hk_image_Subscriber::hk_image_Subscriber()
    : participant_(nullptr)
    , subscriber_(nullptr)
    , topic_(nullptr)
    , reader_(nullptr)
    , type_(new hk_imagePubSubType())
{
}

bool hk_image_Subscriber::init(
        const std::string& wan_ip,
        unsigned short port,
        bool use_tls,
        const std::vector<std::string>& whitelist)
{

    //CREATE THE PARTICIPANT
    DomainParticipantQos pqos;
    int32_t kind = LOCATOR_KIND_TCPv4;

    Locator initial_peer_locator;
    initial_peer_locator.kind = kind;

    std::shared_ptr<TCPv4TransportDescriptor> descriptor = std::make_shared<TCPv4TransportDescriptor>();

    for (std::string ip : whitelist)
    {
        descriptor->interfaceWhiteList.push_back(ip);
        std::cout << "Whitelisted " << ip << std::endl;
    }

    if (!wan_ip.empty())
    {
        IPLocator::setIPv4(initial_peer_locator, wan_ip);
        std::cout << wan_ip << ":" << port << std::endl;
    }
    else
    {
        IPLocator::setIPv4(initial_peer_locator, "127.0.0.1");
    }
    initial_peer_locator.port = port;
    pqos.wire_protocol().builtin.initialPeersList.push_back(initial_peer_locator); // Publisher's meta channel

    pqos.wire_protocol().builtin.discovery_config.leaseDuration = eprosima::fastrtps::c_TimeInfinite;
    pqos.wire_protocol().builtin.discovery_config.leaseDuration_announcementperiod = Duration_t(5, 0);
    pqos.name("Participant_sub");

    // pqos.transport().use_builtin_transports = false;//原来的，置为false将禁用默认的UDPv4实现。注意：和共享内存方案一样，导致monitor没有话题。
    pqos.transport().use_builtin_transports = true;//我手动改为true，不知道是否有影响？

    if (use_tls)
    {
        using TLSVerifyMode = TCPTransportDescriptor::TLSConfig::TLSVerifyMode;
        using TLSOptions = TCPTransportDescriptor::TLSConfig::TLSOptions;
        descriptor->apply_security = true;
        descriptor->tls_config.verify_file = "cacert.pem";
        descriptor->tls_config.verify_mode = TLSVerifyMode::VERIFY_PEER;
        descriptor->tls_config.add_option(TLSOptions::DEFAULT_WORKAROUNDS);
    }

    pqos.transport().user_transports.push_back(descriptor);

    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pqos);
    if (participant_ == nullptr)
    {
        return false;
    }

    //REGISTER THE TYPE
    type_.register_type(participant_);

    //CREATE THE SUBSCRIBER
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    if (subscriber_ == nullptr)
    {
        return false;
    }

    //CREATE THE TOPIC
    topic_ = participant_->create_topic("hk_image_TCP_topic", "hk_image", TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr)
    {
        return false;
    }

    //CREATE THE DATAREADER
    DataReaderQos rqos;
    rqos.history().kind = eprosima::fastdds::dds::KEEP_LAST_HISTORY_QOS;
    rqos.history().depth = 30;
    rqos.resource_limits().max_samples = 50;
    rqos.resource_limits().allocated_samples = 20;
    rqos.reliability().kind = eprosima::fastdds::dds::RELIABLE_RELIABILITY_QOS;
    rqos.durability().kind = eprosima::fastdds::dds::TRANSIENT_LOCAL_DURABILITY_QOS;
    reader_ = subscriber_->create_datareader(topic_, rqos, &listener_);
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
        std::cout << "[RTCP] Subscriber matched" << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        matched_ = info.total_count;
        std::cout << "[RTCP] Subscriber unmatched" << std::endl;
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
