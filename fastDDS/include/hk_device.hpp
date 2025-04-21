#ifndef HK_DEVICE_HPP
#define HK_DEVICE_HPP
#include <stdio.h>
#include <pthread.h>
#include <iostream>
#include <fstream>
#include <yaml-cpp/yaml.h>  // 引入 yaml-cpp 头文件，解析相机配置参数

#include "MvErrorDefine.h"
#include "CameraParams.h"
#include "MvCameraControl.h"

namespace camera
{
    enum CamerProperties
    {
        CAP_FRAMERATE_ENABLE,  //帧数可调
        CAP_FRAMERATE,         //帧数
        CAP_BURSTFRAMECOUNT,   //外部一次触发帧数
        CAP_HEIGHT,            //图像高度
        CAP_WIDTH,             //图像宽度
        CAP_EXPOSURE_TIME,     //固定曝光模式下的曝光时间
        CAP_EXPOSURE_LOWER_TIME,//自动曝光模式下的曝光时间下限
        CAP_EXPOSURE_UPPER_TIME,//自动曝光模式下的曝光时间上限
        CAP_GAMMA_ENABLE,      //伽马因子可调
        CAP_GAMMA,             //伽马因子
        CAP_GAINAUTO,          //亮度
        CAP_SATURATION_ENABLE, //饱和度可调
        CAP_SATURATION,        //饱和度
        CAP_OFFSETX,           //X偏置
        CAP_OFFSETY,           //Y偏置
        CAP_TRIGGER_MODE,      //外部触发
        CAP_TRIGGER_SOURCE,    //触发源
        CAP_LINE_SELECTOR,     //触发线
        CAP_BalanceRatio,      //白平衡
    };


    class Camera
    {
    public:
        Camera();
        ~Camera();
        bool set(camera::CamerProperties type, float value);

        unsigned char *get_pData() const{
            return  pData;
        }
        MV_FRAME_OUT_INFO_EX* get_stImageInfo(){
            return &stImageInfo;
        }
        unsigned int get_nDataSize() const{
            return nDataSize;
        }
        // void *get_handle() const{
        void *get_handle() {
             return handle;
        }
        unsigned char* get_p_DataForRGB(){
            return p_DataForRGB;
        }
        MV_CC_PIXEL_CONVERT_PARAM* get_stConvertParam(){
            return &stConvertParam;
        }


    private:

        //yaml配置文件的参数
        int width;
        int height;
        int Offset_x;
        int Offset_y;
        int GainAuto;
        bool FrameRateEnable;
        int FrameRate;
        bool GammaEnable;
        float Gamma;
        bool SaturationEnable;
        int Saturation;
        int ExposureAuto;
        int ExposureTime;
        int AutoExposureTimeLower;
        int AutoExposureTimeUpper;
        int TriggerMode;
        int TriggerSource;
        int LineSelector;
        int BalanceRatio;

        void *handle;
        int nRet;
        std::string serial_number;//找到的相机序列号
        std::string expect_serial_number;//希望连接的相机序列号
        bool find_expect_camera = false;
        int expect_camera_index = -1;
        int image_empty_count = 0; // 连续空图帧数
        unsigned int nDataSize; //缓冲区的大小
        unsigned char *pData; // 内存指针，指向原始图像数据的内存位置
        unsigned char *p_DataForRGB;//内存指针，指向“将pData数据转化为rgb8格式“后的内存位置
        MV_CC_PIXEL_CONVERT_PARAM stConvertParam = { 0 };         // 定义像素格式转换输入输出参数
        MV_FRAME_OUT_INFO_EX stImageInfo = {0};//该结构体用于存储图像的相关信息，例如图像宽度、高度、帧率等。
    };

    Camera::Camera()
    {
        handle = NULL;//相机设备句柄,初始化为NULL

        // std::ifstream fin("config/my_camera.yaml");  // 打开 YAML 文件
        YAML::Node config = YAML::LoadFile("../config/my_camera.yaml");  // 加载 YAML 文件到 Node 对象

        // 读取yaml文件各个变量的值
        expect_serial_number = config["expect_serial_number"].as<std::string>();
        width = config["width"].as<int>();
        height = config["height"].as<int>();
        Offset_x = config["Offset_x"].as<int>();
        Offset_y = config["Offset_y"].as<int>();
        Gamma = config["Gamma"].as<float>();
        GainAuto = config["GainAuto"].as<int>();
        FrameRateEnable = config["FrameRateEnable"].as<bool>();
        FrameRate = config["FrameRate"].as<int>();
        GammaEnable = config["GammaEnable"].as<bool>();
        SaturationEnable = config["SaturationEnable"].as<bool>();
        Saturation = config["Saturation"].as<int>();
        TriggerMode = config["TriggerMode"].as<int>();
        TriggerSource = config["TriggerSource"].as<int>();
        LineSelector = config["LineSelector"].as<int>();
        ExposureAuto = config["ExposureAuto"].as<int>();
        ExposureTime = config["ExposureTime"].as<int>();
        AutoExposureTimeLower = config["AutoExposureTimeLower"].as<int>();
        AutoExposureTimeUpper = config["AutoExposureTimeUpper"].as<int>();
        BalanceRatio = config["BalanceRatio"].as<int>();

        // 枚举设备 
        MV_CC_DEVICE_INFO_LIST stDeviceList;
        memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
        nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);// 枚举检测到的相机数量
        if (MV_OK != nRet)
        {
            printf("MV_CC_EnumDevices fail! nRet [%x]\n", nRet);
            exit(-1);
        }
        if (stDeviceList.nDeviceNum == 0) {
            printf("Find No Devices!\n");
            exit(-1);
        }else{ // 根据serial number启动指定相机
            for (int i = 0; i < stDeviceList.nDeviceNum; i++) {
                serial_number = std::string((char *)stDeviceList.pDeviceInfo[i]->SpecialInfo.stUsb3VInfo.chSerialNumber);
                if (expect_serial_number == serial_number) {
                    find_expect_camera = true;
                    expect_camera_index = i;
                    break;
                }
            }
        }
        if (!find_expect_camera) {
            printf("Can not find the camera with some serial_number.\n");
            exit(-1);
        }else{
            printf("Find the camera with some serial_number: %s \n", expect_serial_number.c_str());
        }

        //选择设备并创建句柄
        nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[expect_camera_index]);
        if (MV_OK != nRet)
        {
            printf("MV_CC_CreateHandle fail! nRet [%x]\n", nRet);
            exit(-1);
        }
        // 打开设备
        nRet = MV_CC_OpenDevice(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_OpenDevice fail! nRet [%x]\n", nRet);
            exit(-1);
        }

        //配置相机参数
        this->set(CAP_FRAMERATE_ENABLE, FrameRateEnable);
        if (FrameRateEnable)
            this->set(CAP_FRAMERATE, FrameRate);
        this->set(CAP_HEIGHT, height);
        this->set(CAP_WIDTH, width);
        this->set(CAP_OFFSETX, Offset_x);
        this->set(CAP_OFFSETY, Offset_y);
        this->set(CAP_GAMMA_ENABLE, GammaEnable);
        if (GammaEnable)
            this->set(CAP_GAMMA, Gamma);
        this->set(CAP_GAINAUTO, GainAuto);
        this->set(CAP_SATURATION_ENABLE, SaturationEnable);
        if (SaturationEnable)
            this->set(CAP_SATURATION, Saturation);
        this->set(CAP_BalanceRatio,BalanceRatio);
        this->set(CAP_TRIGGER_MODE, TriggerMode);
        // this->set(CAP_TRIGGER_SOURCE, TriggerSource);
        // this->set(CAP_LINE_SELECTOR, LineSelector);
        if(ExposureAuto == 0)//固定曝光模式
        {
            this->set(CAP_EXPOSURE_TIME, ExposureTime);
        }else if (ExposureAuto == 2)//自动曝光模式
        {
            this->set(CAP_EXPOSURE_LOWER_TIME,AutoExposureTimeLower);
            this->set(CAP_EXPOSURE_UPPER_TIME,AutoExposureTimeUpper);
        }

        //********** 设置图像格式 （这里要单独拿出来，不要用set函数。）**********/
        nRet = MV_CC_SetEnumValue(handle, "PixelFormat", 0x02180014);
        if (MV_OK == nRet) {
            MVCC_ENUMVALUE t = {0};
            nRet = MV_CC_GetEnumValue(handle, "PixelFormat", &t);
            if (MV_OK == nRet && t.nCurValue==35127316){
                printf("set PixelFormat OK ! value = RGB\n");
            }else{
                    printf("get PixelFormat fail! nRet [%x]\n", nRet);
            }
        } else {
            printf("set PixelFormat fail! nRet [%x]\n", nRet);
        }

        // 开始取流
        nRet = MV_CC_StartGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_StartGrabbing fail! nRet [%x]\n", nRet);
            exit(-1);
        }

        MVCC_INTVALUE stParam;
        memset(&stParam, 0, sizeof(MVCC_INTVALUE));
        nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stParam);
        if (MV_OK != nRet) {
            printf("Get PayloadSize fail! nRet [0x%x]\n", nRet);
            nDataSize = 1024*3840;//==3932160
        }else{
            //将缓冲区的大小保存在变量nDataSize中，以便后续分配内存使用。
            nDataSize = stParam.nCurValue;
        }
        printf("缓冲区大小 nDataSize == %d\n", nDataSize);

    
        // 分配内存用于存储从相机获取的原始图像数据
        pData = (unsigned char *)malloc(sizeof(unsigned char) * nDataSize);
        if (NULL == pData) {
            printf(" pData == NULL \n");
        }

        p_DataForRGB = (unsigned char*)malloc(height * width * 3);
        if (p_DataForRGB == nullptr) {
            // 处理内存分配失败的情况
            std::cerr << "Memory allocation failed." << std::endl;
            exit(-1);
        }

        // 像素格式转换
		memset(&stConvertParam, 0, sizeof(MV_CC_PIXEL_CONVERT_PARAM));

        memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));//初始化为0，用于存储图像的相关信息，例如图像宽度、高度、帧率等。        
    }

    Camera::~Camera()
    {
        int nRet;
        nRet = MV_CC_StopGrabbing(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_StopGrabbing fail! nRet [%x]\n", nRet);
            exit(-1);
        }
        printf("MV_CC_StopGrabbing succeed.\n");
        // 关闭设备
        nRet = MV_CC_CloseDevice(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_CloseDevice fail! nRet [%x]\n", nRet);
            exit(-1);
        }
        printf("MV_CC_CloseDevice succeed.\n");
        nRet = MV_CC_DestroyHandle(handle);
        if (MV_OK != nRet)
        {
            printf("MV_CC_DestroyHandle fail! nRet [%x]\n", nRet);
            exit(-1);
        }
        printf("MV_CC_DestroyHandle succeed.\n");
    }

    //根据参数类型设置相机参数
    bool Camera::set(CamerProperties type, float value)
    {
        switch (type)
        {
        case CAP_EXPOSURE_TIME:
        {
            nRet = MV_CC_SetFloatValue(handle, "ExposureTime", value); //曝光时间
            if (MV_OK == nRet)
            {
                printf("set ExposureTime OK! value=%f\n",value);
            }
            else
            {
                printf("Set ExposureTime Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_EXPOSURE_LOWER_TIME:
        {
            nRet = MV_CC_SetAutoExposureTimeLower(handle, value);
            if (MV_OK == nRet) {
            printf("Set Exposure Time Lower: %f ms \n", value);
            } else {
            printf("Fail to set Exposure Time Lower");
            }
            break;
        }
        case CAP_EXPOSURE_UPPER_TIME:
        {
            nRet = MV_CC_SetAutoExposureTimeUpper(handle, value);
            if (MV_OK == nRet) {
            printf("Set Exposure Time Upper: %f ms \n", value);
            } else {
            printf("Fail to set Exposure Time Upper");
            }
            break;
        }
        case CAP_FRAMERATE_ENABLE:
        {
            nRet = MV_CC_SetBoolValue(handle, "AcquisitionFrameRateEnable", value);
            if (MV_OK == nRet)
            {
                printf("set AcquisitionFrameRateEnable OK! value=%f\n",value);
            }
            else
            {
                printf("Set AcquisitionFrameRateEnable Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_FRAMERATE:
        {
            nRet = MV_CC_SetFloatValue(handle, "AcquisitionFrameRate", value);
            if (MV_OK == nRet)
            {
                printf("set AcquisitionFrameRate OK! value=%f\n",value);
            }
            else
            {
                printf("Set AcquisitionFrameRate Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_BURSTFRAMECOUNT:
        {
            nRet = MV_CC_SetIntValue(handle, "AcquisitionBurstFrameCount", value);
            if (MV_OK == nRet)
            {
                printf("set AcquisitionBurstFrameCount OK!\n");
            }
            else
            {
                printf("Set AcquisitionBurstFrameCount Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_HEIGHT:
        {
            nRet = MV_CC_SetIntValue(handle, "Height", value); 
            if (MV_OK == nRet)
            {
                printf("set Height OK! value = %f\n",value);
            }
            else
            {
                printf("Set Height Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_WIDTH:
        {
            nRet = MV_CC_SetIntValue(handle, "Width", value); 
            if (MV_OK == nRet)
            {
                printf("set Width OK! value = %f\n",value);
            }
            else
            {
                printf("Set Width Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_OFFSETX:
        {
            nRet = MV_CC_SetIntValue(handle, "OffsetX", value); 

            if (MV_OK == nRet)
            {
                printf("set Offset X OK! value = %f\n",value);
            }
            else
            {
                printf("Set Offset X Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_OFFSETY:
        {
            nRet = MV_CC_SetIntValue(handle, "OffsetY", value); 
            if (MV_OK == nRet)
            {
                printf("set Offset Y OK!  value = %f\n",value);
            }
            else
            {
                printf("Set Offset Y Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_GAMMA_ENABLE:
        {
            nRet = MV_CC_SetBoolValue(handle, "GammaEnable", value); //伽马因子是否可调  默认不可调（false）
            if (MV_OK == nRet)
            {
                printf("set GammaEnable OK! value=%f\n",value);
            }
            else
            {
                printf("Set GammaEnable Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_GAMMA:
        {
            nRet = MV_CC_SetFloatValue(handle, "Gamma", value); //伽马越小 亮度越大
            if (MV_OK == nRet)
            {
                printf("set Gamma OK! value=%f\n",value);
            }
            else
            {
                printf("Set Gamma Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_GAINAUTO:
        {
            nRet = MV_CC_SetEnumValue(handle, "GainAuto", value); //亮度 越大越亮
            if (MV_OK == nRet)
            {
                printf("set GainAuto OK! value=%f\n",value);
            }
            else
            {
                printf("Set GainAuto Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_SATURATION_ENABLE:
        {
            nRet = MV_CC_SetBoolValue(handle, "SaturationEnable", value);
            if (MV_OK == nRet)
            {
                printf("set SaturationEnable OK! value=%f\n",value);
            }
            else
            {
                printf("Set SaturationEnable Failed! nRet = [%x]\n", nRet);
            }
            break;
        }
        case CAP_SATURATION:
        {
            nRet = MV_CC_SetIntValue(handle, "Saturation", value); 
            if (MV_OK == nRet)
            {
                printf("set Saturation OK! value=%f\n",value);
            }
            else
            {
                printf("Set Saturation Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_TRIGGER_MODE:
        {
            nRet = MV_CC_SetEnumValue(handle, "TriggerMode", value); 
            if (MV_OK == nRet)
            {
                printf("set TriggerMode OK! value=%f\n",value);
            }
            else
            {
                printf("Set TriggerMode Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_TRIGGER_SOURCE:
        {
            nRet = MV_CC_SetEnumValue(handle, "TriggerSource", value);
            if (MV_OK == nRet)
            {
                printf("set TriggerSource OK!\n");
            }
            else
            {
                printf("Set TriggerSource Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_LINE_SELECTOR:
        {
            nRet = MV_CC_SetEnumValue(handle, "LineSelector", value); 
            if (MV_OK == nRet)
            {
                printf("set LineSelector OK!\n");
            }
            else
            {
                printf("Set LineSelector Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        case CAP_BalanceRatio:
        {
            nRet = MV_CC_SetEnumValue(handle, "BalanceWhiteAuto", value);
            if (MV_OK == nRet)
            {
                printf("set BalanceWhiteAuto OK! value = %f\n",value);
            }
            else
            {
                printf("Set BalanceWhiteAuto Failed! nRet = [%x]\n\n", nRet);
            }
            break;
        }
        default:
            return 0;
        }
        return nRet;
    }


} // namespace camera
#endif
