#ifndef ARCSOFT_FACE_PROC_H
#define ARCSOFT_FACE_PROC_H

#include <QString>
#include <iostream>
#include <fstream>
#include <cassert>
#include <direct.h>
#include <stdarg.h>
#include <thread>
#include <mutex>
#include <windows.h>
#include "arcsoft/inc/arcsoft_face_sdk.h"
#include "arcsoft/inc/amcomdef.h"
#include "arcsoft/inc/asvloffscreen.h"
#include "arcsoft/inc/merror.h"
#include "opencv2/opencv.hpp"

extern unsigned char *m_faceRecognition_setting;
typedef void (*VideoDecter)(cv::Mat);

using namespace std;
using namespace cv;

#define APPID	"EsDmpwySmAjngXnDRqhFbqKp5TQ1TGHetq72cskdrDyJ"
#define SDKKey	"4vuKo2iayfmW5xunZySeKa9WSfNxcdkNdCFyjtWweS8L"

#define SafeFree(p) { if ((p)) free(p); (p) = NULL; }
#define SafeArrayDelete(p) { if ((p)) delete [] (p); (p) = NULL; }
#define SafeDelete(p) { if ((p)) delete (p); (p) = NULL; }

#define TYPE_ORI_IMAGE		0
#define TYPE_DETECT_IMAGE	1
#define TYPE_FACESET_IMAGE	2

#define DEBUG

typedef  struct __FaceRecognitionSetting
{
    uint8_t     camera_scan_mode;   //0:不巡航 1:巡航
    uint8_t     layer;              //输入图片分层识别数目
    uint8_t     face_frame_width;   //人脸框线宽
    uint8_t     div_ori_deno;       //分割原始图像的分母
    uint8_t     recognize;          //识别阈值
    uint8_t     face_display_num;   //人脸图像展示数目
    uint8_t     time_interval;      //画面刷新时间间隔
}FaceRecognitionSetting;

typedef  struct __FaceRoiInfoImport
{
    int		count;						//	图像序号
    MInt32	faceOrient;					//	输入图像的角度，可以参考 ArcFaceCompare_OrientCode
    Mat		matFaceROI;					//	人脸图像区域
    Mat     ori;                        //  原始图像
    string  name;                       //  名字
    ASF_FaceFeature	faceFeature;        //	存储待识别人脸图片以及特征
}FaceRoiInfoImport; //存储导入的人脸信息

typedef struct __FaceRoiInfo
{
    int		count;						//	图像序号
    MInt32	faceOrient;					//	输入图像的角度，可以参考 ArcFaceCompare_OrientCode
    Mat		matFaceROI;
    bool    recognize;                  //  人脸信息是否提取成功
}FaceRoiInfo;	//检测人脸图像信息，待比对人脸图像信息

typedef struct __FaceFeatureInfo
{
    int             count;              //  图像序号
    Rect            rect;               //  人脸框位置
    struct
    {
        int         count;              //  识别出来的序号
        float       confidence;         //  识别的置信度
        string      name;               //  识别出来的同学的姓名
        int         indexOf;            //  学生索引
    }Student[3];
    ASF_FaceFeature	faceSampleFeature;  //	存储待识别人脸图片以及特征
}FaceFeatureInfo;	//待识别人脸图像特征及置信度

typedef struct __student
{
    int         index;              //  识别出来的序号
    float       confidence;         //  识别的置信度
    string      name;               //  识别出来的同学的姓名
    int         indexOf;            //  学生索引
}Student_Recognize;

typedef struct __ImageInfo
{
    int		width;						//	宽
    int		height;						//	高
    int		pixelBytes;					//	一个像素对应字节数
    uchar	buf[1920*1080*4];			//	保存图像
}ImageInfo;	//保存图像信息，用来做显示

class thread_guard
{
    thread &t;
public :
    explicit thread_guard(thread& _t) :
        t(_t){}

    ~thread_guard()
    {
        if (t.joinable())
            t.join();
    }

    thread_guard(const thread_guard&) = delete;
    thread_guard& operator=(const thread_guard&) = delete;
};

class CArcSoftFaceProc
{
public:
    CArcSoftFaceProc();
    ~CArcSoftFaceProc();
    public:
        //	图像显示接口函数
        int show_image_convert(int type, int index, ImageInfo* image_info, int width, int height);
        //	图像转换4通道用作显示
        int mat2buf_show(Mat image, ImageInfo* image_info, int width, int height);
        //	日志打印
        int printf_log(const char *str, int res=0);
    public:
        //	arcsoft初始化
        int face_proc_init();
        //	arcsoft关闭
        int face_proc_uninit();
        //	清除检测到的人脸集
        int detected_faceinfo_clear();
        //	待比对人脸集释放
        int faceset_clear();
        //	读取待检测图像
        int face_image_import(const char*  image_path, Mat &res);
        //  读取导入图像
        int face_image_import_origin(const char *image_path, int count);
        //  设置导入图片中识别出的学生名字
        int face_image_import_set_name(const char *path);
        //	人脸检测
        int face_detect(Mat &res);
        //  检测并提取导入的证件照信息
        int face_detect_import();
        //  清空导入的证件照信息
        void clear_import_data();
        //  识别导入的人脸
        int face_recognition_import();
        //  读取待检测视频
        int face_video_import(const char *video_path, VideoDecter video_face);

        //  分割字符串
        int split(string src, vector<string> &dest, char delim = ':');
        //  对前3个置信度进行排序
        bool static compare(const Student_Recognize &a, const Student_Recognize &b);
        //  清除所有信息
        void clearAll();
        //  设置容器大小
        void setImportVecSize(int size);
        //  获取当前帧
        void getCurrentFrame(Mat &frame);
        //  视频在线程中播放
        static void move_face_detect_to_thread(void *arg);

    public:
        Mat m_current_frame;                                    //  视频当前帧
        Mat m_ori_detect_img;									//	待检测原图像
        Mat m_ori_recog_img;									//	待识别原图像
        MRESULT m_arcsoft_res;									//  arcsoft函数返回值
        MHandle m_arcsoft_handle;								//	虹软算法句柄
        int m_count;											//	检测到人脸的计数变量
        int m_samplesNums;										//	添加待比对人脸样本的计数变量
        vector<FaceRoiInfo> m_vecFaceInfo;					//	存储检测到的人脸图像
        vector<FaceRoiInfoImport>   m_vecFaceInfoImport;    //  存储导入的人脸图像信息
        vector<FaceFeatureInfo> m_vecFaceFeatureInfo;		//	存储待比对人脸图像特征及置信度
#ifdef DEBUG
        string m_str_log;										//	Debug调试使用
#endif

        //视频
        VideoCapture cap;
        long m_totalFrameNum;
        long m_currentFrameNum;
        VideoDecter m_video_detecter;
        mutex m_tx;
};

#endif // ARCSOFT_FACE_PROC_H
