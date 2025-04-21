#include "hk_imagePubSubTypes.h"

#include <chrono>
#include <thread>
#include <opencv2/opencv.hpp>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;

class hk_image_Subscriber 
{
private:
    DomainParticipant* participant_;
    Subscriber* subscriber_;
    DataReader* reader_;
    Topic* topic_;
    TypeSupport type_;
    class SubListener : public DataReaderListener
    {
    public:
        SubListener()
         {
         }
        ~SubListener() override
         {
         }
        void on_subscription_matched(
                 DataReader*,
                 const SubscriptionMatchedStatus& info) override
        {
            if (info.current_count_change == 1)
            {
                std::cout << "Subscriber matched." << std::endl;
            }
            else if (info.current_count_change == -1)
            {
                std::cout << "Subscriber unmatched." << std::endl;
            }
            else
            {
                std::cout << info.current_count_change
                         << " is not a valid value for SubscriptionMatchedStatus current count change" << std::endl;
            }
        }
 
        void on_data_available(
                DataReader* reader) override
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
 
        hk_image hk_image_;
    } listener_;

public:
    hk_image_Subscriber()
        : participant_(nullptr)
        , subscriber_(nullptr)
        , topic_(nullptr)
        , reader_(nullptr)
        , type_(new hk_imagePubSubType())
    {
    }

    virtual ~hk_image_Subscriber()
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

    bool init()
    {
        DomainParticipantQos participantQos;
        participantQos.name("Participant_subscriber");
        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

        if (participant_ == nullptr)
        {
            return false;
        }

        // Register the Type
        type_.register_type(participant_);

        // Create the subscriptions Topic
        topic_ = participant_->create_topic("hk_image_topic", "hk_image", TOPIC_QOS_DEFAULT);
        if (topic_ == nullptr)
        {
            return false;
        }

        // Create the Subscriber
        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
        if (subscriber_ == nullptr)
        {
            return false;
        }

        // Create the DataReader
        reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, &listener_);
        if (reader_ == nullptr)
        {
            return false;
        }

        return true;
   }


};

int main(int argc, char** argv)
{
    std::cout << "Starting subscriber hk_image..." << std::endl;

    hk_image_Subscriber* mysub = new hk_image_Subscriber();
    if(mysub->init())
    {
        std::cout<<"init() 完成！开始接收消息..."<<std::endl;
        while(1){
            std::this_thread::sleep_for(std::chrono::milliseconds(5));//主函数休眠，在mysub里面接收到消息后调用回调函数on_data_available（）。这里处理的不是很好。。。
        }
    }

    delete mysub;
    std::cout<<"退出程序..."<<std::endl;
    return 0;
}