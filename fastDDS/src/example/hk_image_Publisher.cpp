// #include "hk_imagePubSubTypes.hpp"
#include "hk_imagePubSubTypes.h"

#include "../include/hk_device.hpp"

#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;

class hk_image_Publisher
{
private:
    hk_image hk_image_;
    DomainParticipant* participant_;
    Publisher* publisher_;
    Topic* topic_;
    DataWriter* writer_;
    TypeSupport type_;//将用于在 DomainParticipant 中注册主题数据类型的对象。

    camera::Camera MVS_cap;//相机设备


    class PubListener:public DataWriterListener{
    public:
        PubListener():matched_(0){ }
        ~PubListener() override{ }

// 重写 DataWriter 侦听器回调函数。当检测到新的 DataReader 侦听 DataWriter 正在发布的主题时，执行回调函数
        void on_publication_matched(
            DataWriter*,
            const PublicationMatchedStatus& info) override{
                if(info.current_count_change == 1)
                {
                    matched_ = info.total_count;
                    std::cout<<"publisher matched."<<std::endl;
                }
                else if (info.current_count_change == -1)
                {
                    matched_ = info.total_count;
                    std::cout << "Publisher unmatched." << std::endl;
                }
                else
                {
                    std::cout << info.current_count_change
                            << " is not a valid value for PublicationMatchedStatus current count change." << std::endl;
                }
            }
        

        std::atomic_int matched_;

    }listener_;

public:
    hk_image_Publisher()
        : participant_(nullptr)
        , publisher_(nullptr)
        , topic_(nullptr)
        , writer_(nullptr)
        , type_(new hk_imagePubSubType())
    {
    }
    virtual ~hk_image_Publisher()
    {
        //participant_里面是topic和publisher_，publisher_里面是writer_
        if (writer_ != nullptr){
            publisher_->delete_datawriter(writer_);
        }
        if(publisher_ != nullptr){
            participant_->delete_publisher(publisher_);
        }
        if(topic_ != nullptr){
            participant_->delete_topic(topic_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }

    //!Initialize
    //依次实例化类：participant_里面是topic和publisher_，publisher_里面是writer_
    //除participant_的name外，所有实体的 QoS 配置都是默认配置
    bool init()
    {
        hk_image_.index(0);
        hk_image_.message("hk_image_message...");


        //不能这样做，会报错：段错误 (核心已转储),可能涉及到ulimit -s 堆栈内存
        // std::array<char, 10 * 1024 * 1024> image_data_array;//默认全部填充为0
        // std::cout<<a++<<std::endl;
        // hk_image_.image_data(image_data_array);

        DomainParticipantQos participantQos;
        participantQos.name("Participant_publisher");
        participant_ = DomainParticipantFactory::get_instance()->create_participant(0,participantQos);
        if(participant_ == nullptr){
            return false;
        }

        // Register the Type
        type_.register_type(participant_);

        // Create the publications Topic
        topic_ = participant_->create_topic("hk_image_topic","hk_image",TOPIC_QOS_DEFAULT);//使用默认的QoS
        if(topic_ == nullptr){
            return false;
        }

        // Create the Publisher
        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT,nullptr);
        if(publisher_ == nullptr) {
            return false;
        }

        // 使用先前创建的侦听器(listener_)创建DataWriter。
        writer_ = publisher_->create_datawriter(topic_,DATAWRITER_QOS_DEFAULT,&listener_);
        if(writer_ == nullptr){
            return false;
        }

        return true;
    }

    bool start_publish()
    {
        int image_empty_count = 0;
        int nRet;
        while(true)
        {
            // 从相机中获取一帧图像，存放到pData里，图像信息放在stImageInfo，超时时间20毫秒
            nRet = MV_CC_GetOneFrameTimeout(MVS_cap.get_handle(), MVS_cap.get_pData(), MVS_cap.get_nDataSize(), MVS_cap.get_stImageInfo(), 20);
            if (nRet != MV_OK){
                std::cout<<"MV_CC_GetOneFrameTimeout failed number: "<<image_empty_count<<std::endl;
                if (++image_empty_count > 100){
                    std::cout<<"The Number of Faild Reading Exceed The Set Value: "<<image_empty_count<<std::endl;
                    exit(-1);//连续100张空图就退出
                }
                continue;
            }
            image_empty_count = 0; 
            // 从设备获取的源数据    
            MVS_cap.get_stConvertParam()->nWidth = MVS_cap.get_stImageInfo()->nWidth;              // 图像宽 
            MVS_cap.get_stConvertParam()->nHeight = MVS_cap.get_stImageInfo()->nHeight;            // 图像高 
            MVS_cap.get_stConvertParam()->pSrcData = MVS_cap.get_pData();                          // 输入数据缓存  
            MVS_cap.get_stConvertParam()->nSrcDataLen = MVS_cap.get_stImageInfo()->nFrameLen;      // 输入数据大小 
            MVS_cap.get_stConvertParam()->enSrcPixelType = MVS_cap.get_stImageInfo()->enPixelType; // 输入数据像素格式 
            // 转换像素格式后的目标数据
            MVS_cap.get_stConvertParam()->enDstPixelType = PixelType_Gvsp_BGR8_Packed;             // 输出像素格式 
            MVS_cap.get_stConvertParam()->pDstBuffer = MVS_cap.get_p_DataForRGB();                 // 输出数据缓存 
            MVS_cap.get_stConvertParam()->nDstBufferSize = 3 * MVS_cap.get_stImageInfo()->nHeight * MVS_cap.get_stImageInfo()->nWidth; // 缓存大小（输入？）
            //将采集到的原始数据转换为像素格式并保存到指定的内存中。
            nRet = MV_CC_ConvertPixelType(MVS_cap.get_handle(), MVS_cap.get_stConvertParam());
            if (MV_OK != nRet){
                printf("-----ERROR: MV_CC_ConvertPixelType failed! , [%x] \n", nRet);//报错：nRet==0x80000000：错误或无效句柄，重启！
            }

            if(listener_.matched_ > 0 ){//当前正在监听我的节点的数量>0(有人在听我，我才发送数据。不然就不发送，节省能量)
                hk_image_.index(hk_image_.index() + 1);
                hk_image_.message("message");
                hk_image_.height(MVS_cap.get_stImageInfo()->nHeight);
                hk_image_.width(MVS_cap.get_stImageInfo()->nWidth);
                auto now = std::chrono::system_clock::now();
                auto duration = now.time_since_epoch();
                auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
                uint64_t current_time_milliseconds = static_cast<uint64_t>(milliseconds);
                hk_image_.timestamp(current_time_milliseconds);


                //图像数据：MVS_cap.get_p_DataForRGB()，它是指针（unsigned char *，指向大小为height * width * 3的内存），hk_image::image_data要求输入是“std::array<uint8_t, 3*1280*1024>&”类型。
                //图像大小：3 * MVS_cap.get_stImageInfo()->nHeight * MVS_cap.get_stImageInfo()->nWidth
                
                /*
                    方案一：设置.idl文件中的传递图像数据的变量类型为std::array<uint8_t,3*1280*1024>
                    由于 std::array 是一个固定大小的容器，不能直接将指针（MVS_cap.get_p_DataForRGB()）赋值给它。需要将指针指向的数据复制（std::copy）到 std::array 中。
                */
                // // 计算图像数据大小
                // size_t image_size = 3 * MVS_cap.get_stImageInfo()->nHeight * MVS_cap.get_stImageInfo()->nWidth;
                // // 确保图像大小不超过 std::array 的容量
                // constexpr size_t array_capacity = 3 * 1280 * 1024;
                // if (image_size != array_capacity) {
                //     std::cerr << "Error: 图像数据大小和std::array大小不匹配,无法复制." << std::endl;
                //     exit(-1);
                // }
                std::array<uint8_t, 3*1280*1024> temp_array;
                std::copy(MVS_cap.get_p_DataForRGB(), MVS_cap.get_p_DataForRGB() + (3 * MVS_cap.get_stImageInfo()->nHeight * MVS_cap.get_stImageInfo()->nWidth), temp_array.begin());
                hk_image_.image_data(temp_array);

                /*
                    方案二：设置.idl文件中的传递图像数据的变量类型为std::vector<uint8_t>
                */
                // std::vector<uint8_t> temp_Vector(MVS_cap.get_p_DataForRGB(), MVS_cap.get_p_DataForRGB() + 3 * MVS_cap.get_stImageInfo()->nHeight * MVS_cap.get_stImageInfo()->nWidth);
                // hk_image_.image_data(temp_Vector);

                writer_->write(&hk_image_);//把数据发送出去。
                std::cout<<"发送成功"<<std::endl;
            }else{
                std::cout << "等待订阅者..." << std::endl;
            }
        }
    }

};


int main(int argc,char** argv){
    
    hk_image_Publisher* mypub = new hk_image_Publisher();
    if(mypub->init()){
        std::cout<<"init() 完成！开始发布..."<<std::endl;
        mypub->start_publish();
    }

    delete mypub;
    return 0;
}
