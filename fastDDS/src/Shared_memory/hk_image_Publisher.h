#include "hk_imagePubSubTypes.h"
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/topic/Topic.hpp>

#include "hk_image.h"
// #include "../include/hk_device.hpp"
#include "../../include/hk_device.hpp"

using namespace eprosima::fastdds::dds;

class hk_image_Publisher
{
public:

    hk_image_Publisher();
    virtual ~hk_image_Publisher();
    bool init();
    bool start_publish();

private:
    hk_image hk_image_;
    // std::shared_ptr<hk_image> hk_image_;

    DomainParticipant* participant_;
    Publisher* publisher_;
    Topic* topic_;
    DataWriter* writer_;
    bool stop_;
    camera::Camera MVS_cap;//相机设备

    class PubListener : public DataWriterListener
    {
    public:
        PubListener()  : matched_(0) 
        {
        }
        ~PubListener() override
        {
        }
        void on_publication_matched(
                DataWriter* writer,
                const PublicationMatchedStatus& info) override;
        int matched_;
    } listener_;

    TypeSupport type_;
};
