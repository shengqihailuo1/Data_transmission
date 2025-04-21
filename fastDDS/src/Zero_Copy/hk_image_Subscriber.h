#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

#include <opencv2/opencv.hpp>

using namespace eprosima::fastdds::dds;

class hk_image_Subscriber
{
public:
    hk_image_Subscriber();
    virtual ~hk_image_Subscriber();
    bool init();

    class SubListener : public DataReaderListener
    {
    public:
        SubListener() = default;
        ~SubListener() override = default;

        void on_data_available(DataReader* reader) override;

        void on_subscription_matched(
                DataReader* reader,
                const SubscriptionMatchedStatus& info) override;

        int matched_ = 0;
        //hk_image hk_image_;
    }
    listener_;
    

private:
    DomainParticipant* participant_;
    Subscriber* subscriber_;
    Topic* topic_;
    DataReader* reader_;
    TypeSupport type_;
};

