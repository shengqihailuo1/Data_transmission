#include "hk_image_Subscriber.h"

#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastrtps/attributes/ParticipantAttributes.h>
#include <fastrtps/attributes/SubscriberAttributes.h>

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
    DomainParticipantQos pqos;
    pqos.name("Participant_sub");
    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pqos);
    if (participant_ == nullptr)
    {
        return false;
    }

    //REGISTER THE TYPE
    type_.register_type(participant_);

    //CREATE THE SUBSCRIBER
    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

    if (subscriber_ == nullptr)
    {
        return false;
    }

    //CREATE THE TOPIC
    topic_ = participant_->create_topic(
        "hk_image_DataSharing",
        "hk_image",
        TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr)
    {
        return false;
    }

    // CREATE THE READER
    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
    rqos.durability().kind = VOLATILE_DURABILITY_QOS;
    rqos.data_sharing().automatic();
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
        if (info.instance_state == ALIVE_INSTANCE_STATE)
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
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));//主函数休眠，在mysub里面接收到消息后调用回调函数on_data_available（）。这里处理的不是很好。。。
        }
    }

    delete my_sub;
    std::cout<<"退出程序..."<<std::endl;
    return 0;
}