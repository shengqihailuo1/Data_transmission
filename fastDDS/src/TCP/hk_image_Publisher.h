#ifndef HK_IMAGE_PUBLISHER_H
#define HK_IMAGE_PUBLISHER_H

#include "hk_imagePubSubTypes.h"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>

#include "hk_image.h"
// #include "../../include/hk_device.hpp" //放在.cpp文件中

#include <vector>

using namespace eprosima::fastdds::dds;

class hk_image_Publisher
{
    class PubListener : public DataWriterListener
    {
    public:

        PubListener() : matched_(0)
        {
        }

        ~PubListener() override
        {
        }

        void on_publication_matched(
                DataWriter* writer,
                const PublicationMatchedStatus& info) override;

        int matched_;
    }
    listener_;

    hk_image hk_image_;
    // std::shared_ptr<hk_image> hk_image_;

    DomainParticipant* participant_;
    eprosima::fastdds::dds::Publisher* publisher_;
    Topic* topic_;
    DataWriter* writer_;
    bool stop_;
    TypeSupport type_;
    // camera::Camera MVS_cap;//相机设备


public:

    hk_image_Publisher();
    virtual ~hk_image_Publisher();
    bool init(
            const std::string& wan_ip,
            unsigned short port,
            bool use_tls,
            const std::vector<std::string>& whitelist);
    bool start_publish();
};

#endif