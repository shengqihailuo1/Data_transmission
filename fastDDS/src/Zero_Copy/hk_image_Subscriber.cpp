#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>

#include "hk_image_Subscriber.h"
#include "hk_imagePubSubTypes.h"
#include <thread>

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
    pqos.name("Participant_sub");
    participant_ = DomainParticipantFactory::get_instance()->create_participant(0, pqos);
    if (participant_ == nullptr)
    {
        return false;
    }

    type_.register_type(participant_);

    subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);
    if (subscriber_ == nullptr)
    {
        return false;
    }

    topic_ = participant_->create_topic("hk_image_ZeroCopy_Topic",type_.get_type_name(), TOPIC_QOS_DEFAULT);
    if (topic_ == nullptr)
    {
        return false;
    }

    //配置DATAREADER数据读取者的历史记录、资源限制、可靠性和持久性
    DataReaderQos rqos = subscriber_->get_default_datareader_qos();
    rqos.history().depth = 10;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
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
    FASTDDS_CONST_SEQUENCE(DataSeq, hk_image);
    DataSeq data;
    SampleInfoSeq infos;
    while (ReturnCode_t::RETCODE_OK == reader->take(data, infos))
    {
        for (LoanableCollection::size_type i = 0; i < infos.length(); ++i)
        {
            if (infos[i].valid_data)
            {
                // 通过接收队列获得样本数据的引用
                const hk_image& sample = data[i];

                //这里处理接收到的数据 sample
                //std::cout<<"Sample "<<(reader->is_sample_valid(&sample, &infos[i]) ? " is valid" : " was replaced" ) << std::endl;
                std::cout << "接收到: "<<" index: " << sample.index() <<" width: "<<sample.width()<<" height: "<<sample.height()<< " timestamp: "<<sample.timestamp()<< std::endl;
                // std::cout<< " message: " << sample.message().data() <<std::endl;//Zero-Copy的message类型不能用。见官网文档https://fast-dds.docs.eprosima.com/en/v2.14.3/fastdds/use_cases/zero_copy/zero_copy.html

                // cv::Mat image(1024, 1280, CV_8UC3, const_cast<uint8_t*>(sample.image_data().data()));
                // cv::imshow("Image", image);
                // cv::waitKey(10);
            }
        }
        // 归还缓冲区持有权
        reader->return_loan(data, infos);
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
            std::this_thread::sleep_for(std::chrono::seconds(1));//主函数休眠，在mysub里面接收到消息后调用回调函数on_data_available（）。这里处理的不是很好。。。
        }
    }

    delete my_sub;
    std::cout<<"退出程序..."<<std::endl;
    return 0;
}