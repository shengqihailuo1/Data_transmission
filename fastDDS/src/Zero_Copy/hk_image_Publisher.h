#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "hk_image.h"
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
    // hk_image hk_image_;

    DomainParticipant* participant_;
    Publisher* publisher_;
    Topic* topic_;
    DataWriter* writer_;
    bool stop_;
    TypeSupport type_;
    camera::Camera MVS_cap;//相机设备

    class PubListener : public DataWriterListener
    {
    public:
        PubListener() = default;
        ~PubListener() override = default;

        void on_publication_matched(
                DataWriter* writer,
                const PublicationMatchedStatus& info) override;
        int matched_ = 0;
    } listener_;

};
