#ifndef HK_IMAGE_SUBSCRIBER_H
#define HK_IMAGE_SUBSCRIBER_H

#include "hk_imagePubSubTypes.h"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <vector>

#include <opencv2/opencv.hpp>
#include "hk_image.h"

using namespace eprosima::fastdds::dds;

class hk_image_Subscriber
{
public:

    class SubListener : public DataReaderListener
    {
    public:

        SubListener() : matched_(0)
        {
        }

        ~SubListener() override
        {
        }

        void on_data_available(
                DataReader* reader) override;

        void on_subscription_matched(
                DataReader* reader,
                const SubscriptionMatchedStatus& info) override;

        hk_image hk_image_;
        int matched_;
    }
    listener_;

    hk_image_Subscriber();

    virtual ~hk_image_Subscriber();

    bool init(
            const std::string& wan_ip,
            unsigned short port,
            bool use_tls,
            const std::vector<std::string>& whitelist);

private:
    DomainParticipant* participant_;
    eprosima::fastdds::dds::Subscriber* subscriber_;
    Topic* topic_;
    DataReader* reader_;
    TypeSupport type_;
};

#endif