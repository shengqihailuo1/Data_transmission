#include "hk_imagePubSubTypes.h"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastrtps/subscriber/SampleInfo.h>
#include <fastdds/dds/core/status/SubscriptionMatchedStatus.hpp>

#include <opencv2/opencv.hpp>
#include "hk_image.h"

using namespace eprosima::fastdds::dds;

class hk_image_Subscriber
{
public:

    hk_image_Subscriber();
    virtual ~hk_image_Subscriber();
    bool init();

private:

    DomainParticipant* participant_;
    Subscriber* subscriber_;
    Topic* topic_;
    DataReader* reader_;
    TypeSupport type_;

    class SubListener : public DataReaderListener
    {
    public:
        SubListener(): matched_(0)
        {
        }

        ~SubListener() override
        {
        }

        void on_data_available(DataReader* reader) override;

        void on_subscription_matched(
                DataReader* reader,
                const SubscriptionMatchedStatus& info) override;

        int matched_;
        hk_image hk_image_;
    }
    listener_;
};
