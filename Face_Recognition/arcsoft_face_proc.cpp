#include "arcsoft_face_proc.h"


//为参数分配内存空间
unsigned char *m_faceRecognition_setting;

CArcSoftFaceProc::CArcSoftFaceProc()
{
    m_count = 0;
    m_samplesNums = 0;
}

CArcSoftFaceProc::~CArcSoftFaceProc()
{

}

int CArcSoftFaceProc::show_image_convert(int type, int index, ImageInfo *image_info, int width, int height)
{
    switch (type)
    {
    case TYPE_ORI_IMAGE:
        mat2buf_show(m_ori_detect_img, image_info, width, height);
        break;
    case TYPE_DETECT_IMAGE:
        mat2buf_show(m_vecFaceInfo[index].matFaceROI, image_info, width, height);
        break;
    case TYPE_FACESET_IMAGE:
        break;
    default:
        break;
    }

    return 0;
}

int CArcSoftFaceProc::mat2buf_show(Mat input_image, ImageInfo *image_info, int width, int height)
{
    Mat image;
    resize(input_image, image, Size(width, height));// 缩放Mat并备份
    switch (image.channels())
    {
    case 1:
        cvtColor(image, image, CV_GRAY2BGRA); // GRAY单通道
        break;
    case 3:
        cvtColor(image, image, CV_BGR2BGRA);  // BGR三通道
        break;
    default:
        break;
    }

    image_info->width = image.cols;
    image_info->height = image.rows;
    image_info->pixelBytes = image.channels()*(image.depth() + 1); // 计算一个像素多少个字节
    memcpy(image_info->buf, image.data, image.cols*image.rows*image.channels());

    return 0;
}

int CArcSoftFaceProc::printf_log(const char *str, int res)
{
#ifdef DEBUG
    wchar_t  ws[100];
    swprintf(ws, 100, L"%hs", str);
    string s = str;
    m_str_log = s + ":" + to_string(res);
    cout<<m_str_log<<endl;
#else
    printf("%s: %d\n", res);
#endif
    return 0;
}

int CArcSoftFaceProc::face_proc_init()
{
    //激活SDK
    m_arcsoft_res = ASFActivation((char *)APPID, (char *)SDKKey);
    if (MOK != m_arcsoft_res && MERR_ASF_ALREADY_ACTIVATED != m_arcsoft_res)
    {
        printf_log(string("ALActivation fail").data(), m_arcsoft_res);
        return -1;
    }
    printf_log(string("ALActivation sucess").data(), m_arcsoft_res);

    //初始化引擎
    m_arcsoft_handle = nullptr;
    MInt32 mask = ASF_FACE_DETECT | ASF_FACERECOGNITION | ASF_AGE | ASF_GENDER | ASF_FACE3DANGLE;
    m_arcsoft_res = ASFInitEngine(ASF_DETECT_MODE_IMAGE, ASF_OP_0_HIGHER_EXT, 32, 50, mask, &m_arcsoft_handle);
    if (MOK != m_arcsoft_res)
    {
        printf_log(string("ALInitEngine fail").data(), m_arcsoft_res);
        return -1;
    }
    printf_log(string("ALInitEngine sucess").data(), m_arcsoft_res);

    return 0;
}

int CArcSoftFaceProc::face_proc_uninit()
{
    //反初始化
    m_arcsoft_res = ASFUninitEngine(m_arcsoft_handle);
    if (MOK != m_arcsoft_res)
    {
        printf_log(string("ALUninitEngine fail").data(), m_arcsoft_res);
        return -1;
    }
    printf_log(string("ALUninitEngine sucess").data(), m_arcsoft_res);

    return 0;
}

int CArcSoftFaceProc::detected_faceinfo_clear()
{
    m_count = 0;
    vector<FaceRoiInfo>().swap(m_vecFaceInfo);

    return 0;
}

int CArcSoftFaceProc::faceset_clear()
{
    m_samplesNums = 0;
    for (int idx = 0; idx < m_vecFaceFeatureInfo.size(); idx++)
    {
        if(NULL != m_vecFaceFeatureInfo[idx].faceSampleFeature.feature)
            SafeFree(m_vecFaceFeatureInfo[idx].faceSampleFeature.feature);		//释放内存
    }
    m_vecFaceFeatureInfo.clear();
    vector<FaceFeatureInfo>().swap(m_vecFaceFeatureInfo);

    return 0;
}

int CArcSoftFaceProc::face_image_import(const char *image_path, Mat &res)
{
    int ret = 0;
    m_ori_detect_img = cv::imread(image_path);  // 读取图片
    if (m_ori_detect_img.empty())
    {
        printf("imread is error!\n");
        return -1;
    }
    m_ori_detect_img.copyTo(res);

    //  人脸检测
    face_detect(res);
    ret = face_recognition_import();
    return ret;
}

int CArcSoftFaceProc::face_image_import_origin(const char *image_path, int count)
{
    Mat ori = cv::imread(image_path);
    if(ori.empty())
    {
        printf("imread is error!\n");
        return -2;
    }
    Mat tmp;
    if ((0 != ori.cols % 8) || (0 != ori.rows % 2))
    {
        Rect rect;
        rect.width = ori.cols / 8 * 8;
        rect.height = ori.rows / 2 * 2;
        ori(rect).copyTo(tmp);
    }
    else {
        ori.copyTo(tmp);
    }
    if ((0 != tmp.cols % 8) || (0 != tmp.rows % 2))
    {
        return count;
    }
    if(m_vecFaceInfoImport.size() >= (count + 1))
    {
        m_vecFaceInfoImport[count].count = count;
        tmp.copyTo(m_vecFaceInfoImport[count].ori);
    }
    else
    {
        FaceRoiInfoImport info;
        info.count = count;
        tmp.copyTo(info.ori);
        m_vecFaceInfoImport.push_back(info);
    }
    return -1;
}

int CArcSoftFaceProc::face_image_import_set_name(const char *path)
{
    if(m_vecFaceInfoImport.empty())
    {
        ifstream file;
        file.open(path);
        assert(file.is_open()); //若失败，输出错误信息，并停止运行
        string s;
        while (getline(file, s))
        {
            FaceRoiInfoImport info;
            vector<string> dst;
            split(s, dst, ':');
            info.count = atoi(dst[0].data());
            info.name = dst[1];
            m_vecFaceInfoImport.push_back(info);
        }
        file.close();
    }
    else
    {
        ifstream file;
        file.open(path);
        assert(file.is_open());
        string s;
        int index = 0;
        while(getline(file, s))
        {
            vector<string> dst;
            split(s, dst, ':');
            if(m_vecFaceInfoImport.size() >= (index + 1))
            {
                m_vecFaceInfoImport[index].name = dst[1];
            }
            else
            {
                FaceRoiInfoImport info;
                vector<string> dst;
                split(s, dst, ':');
                info.count = atoi(dst[0].data());
                info.name = dst[1];
                m_vecFaceInfoImport.push_back(info);
            }
            index++;
        }
        file.close();
    }
    return 0;
}

int CArcSoftFaceProc::face_detect(Mat &res)
{
    if (m_ori_detect_img.empty())
    {
        printf("The resolution of Input Image is Empty. \n");
        return -1;
    }
    if ((0 != m_ori_detect_img.cols % 8) || (0 != m_ori_detect_img.rows % 2))
    {
        printf("The resolution of Input Image is Error. \n");
        return -1;
    }

    Mat frameROI;
    Rect rectFace;
    Rect rectROI;
    vector<FaceFeatureInfo> m_tmpVecFace;
    FaceRecognitionSetting *param = (FaceRecognitionSetting *)m_faceRecognition_setting;
    rectROI.width = m_ori_detect_img.cols / param->div_ori_deno;
    rectROI.height = m_ori_detect_img.rows / param->div_ori_deno;
    for (int idx = 0; idx <= m_ori_detect_img.cols / param->div_ori_deno; idx += m_ori_detect_img.cols / param->layer)
    {
        for (int idy = 0; idy <= m_ori_detect_img.rows / param->div_ori_deno; idy += m_ori_detect_img.rows / param->layer)
        {
            ASF_MultiFaceInfo detectedFaces1;
            rectROI.x = idx;
            rectROI.y = idy;

            if((rectROI.x + rectROI.width) > m_ori_detect_img.cols || (rectROI.y + rectROI.height) > m_ori_detect_img.rows)
                break;

            m_ori_detect_img(rectROI).copyTo(frameROI);  // 获取ROI区域

            // 进行人脸检测
            m_arcsoft_res = ASFDetectFaces(m_arcsoft_handle, frameROI.cols, frameROI.rows, ASVL_PAF_RGB24_B8G8R8, (MUInt8*)frameROI.data, &detectedFaces1);
            if (MOK != m_arcsoft_res)
            {
                printf_log(string("ASFDetectFaces fail").data(), m_arcsoft_res);
                continue;
            }

            for (int i = 0; i < detectedFaces1.faceNum; i++)
            {
                FaceRoiInfo faceInfoTmp;
                rectFace.x = detectedFaces1.faceRect[i].left;
                rectFace.y = detectedFaces1.faceRect[i].top;
                rectFace.width = abs(detectedFaces1.faceRect[i].right - detectedFaces1.faceRect[i].left + 1);
                rectFace.height = abs(detectedFaces1.faceRect[i].bottom - detectedFaces1.faceRect[i].top + 1);
                /************ 参数检测：START **************/
                if (rectFace.x < 0)
                    rectFace.x = 0;
                if (rectFace.y < 0)
                    rectFace.y = 0;
                if (rectFace.br().x > frameROI.cols)
                    rectFace.width = frameROI.cols - rectFace.x;
                if (rectFace.br().y > frameROI.rows)
                    rectFace.height = frameROI.rows - rectFace.y;

                if (0 != rectFace.width % 4)
                    rectFace.width = ((rectFace.width + 3) / 4) * 4;
                if (0 != rectFace.height % 2)
                    rectFace.height = ((rectFace.height + 1) / 2) * 2;
                if (rectFace.br().x > frameROI.cols)
                    rectFace.width = rectFace.width - 4;
                if (rectFace.br().y > frameROI.rows)
                    rectFace.height = rectFace.height - 2;
                /************ 参数检测：END **************/
                //Mat tmp;
                //frameROI(rectFace).copyTo(tmp);

                faceInfoTmp.faceOrient = detectedFaces1.faceOrient[i]; // 人脸倾斜角度
                frameROI(rectFace).copyTo(faceInfoTmp.matFaceROI);
                faceInfoTmp.count = m_count;
                m_vecFaceInfo.push_back(faceInfoTmp);
                m_count++;

                //提取检测到的所有人脸特征
                ASF_FaceFeature  feature_face;   // 存储人脸特征
                ASF_SingleFaceInfo SingleDetectedFaces2;
                SingleDetectedFaces2.faceRect.left = 0;
                SingleDetectedFaces2.faceRect.top = 0;
                SingleDetectedFaces2.faceRect.right = faceInfoTmp.matFaceROI.cols-1;
                SingleDetectedFaces2.faceRect.bottom = faceInfoTmp.matFaceROI.rows-1;
                SingleDetectedFaces2.faceOrient = faceInfoTmp.faceOrient;
                m_arcsoft_res = ASFFaceFeatureExtract(m_arcsoft_handle, faceInfoTmp.matFaceROI.cols, faceInfoTmp.matFaceROI.rows, ASVL_PAF_RGB24_B8G8R8, (MUInt8*)faceInfoTmp.matFaceROI.data, &SingleDetectedFaces2, &feature_face);
                if (MOK != m_arcsoft_res)
                {
                    m_vecFaceInfo[m_vecFaceInfo.size() - 1].recognize = false;
                    //增加空位
                    FaceFeatureInfo faceFeatureInfo;
                    faceFeatureInfo.faceSampleFeature.feature = NULL;
                    m_vecFaceFeatureInfo.push_back(faceFeatureInfo);
                    printf_log(string("ASFFaceFeatureExtract fail").data(), m_arcsoft_res);
                    continue;
                }
                m_vecFaceInfo[m_vecFaceInfo.size() - 1].recognize = true;
                //拷贝feature
                FaceFeatureInfo faceFeatureInfo;
                faceFeatureInfo.faceSampleFeature.featureSize = feature_face.featureSize;
                faceFeatureInfo.faceSampleFeature.feature = (MByte *)malloc(feature_face.featureSize);
                memset(faceFeatureInfo.faceSampleFeature.feature, 0, feature_face.featureSize);
                memcpy(faceFeatureInfo.faceSampleFeature.feature, feature_face.feature, feature_face.featureSize);
                faceFeatureInfo.count = m_count - 1;
                {
                    Rect faceRect;
                    faceRect.x = rectFace.x + rectROI.x;
                    faceRect.y = rectFace.y + rectROI.y;
                    faceRect.width = rectFace.width;
                    faceRect.height = rectFace.height;
                    faceFeatureInfo.rect = faceRect;
                }
                m_vecFaceFeatureInfo.push_back(faceFeatureInfo);  // 保存人脸特征

                MFloat confidence = 0;
                MFloat confidentLevel = 0;
                if(m_tmpVecFace.size() > 0)
                {
                    //人脸比对，去除重复的人脸,用于结果图显示
                    for(int i = 0; i < m_tmpVecFace.size(); i++)
                    {
                        m_arcsoft_res = ASFFaceFeatureCompare(m_arcsoft_handle, &feature_face, &m_tmpVecFace[i].faceSampleFeature, &confidence);
                        if (MOK != m_arcsoft_res)
                        {
                            printf_log(string("ASFFaceFeatureCompare fail").data(), m_arcsoft_res);
                        }
                        confidentLevel = MAX(confidence, confidentLevel);
                    }
                }

                if(confidentLevel < (float)param->recognize / 10 || param->div_ori_deno == 1)
                //  提取人脸特征
                {
                    //画检测到的人脸框
                    Rect faceRect;
                    faceRect.x = rectFace.x + rectROI.x;
                    faceRect.y = rectFace.y + rectROI.y;
                    faceRect.width = rectFace.width;
                    faceRect.height = rectFace.height;
                    rectangle(res, faceRect, Scalar(255, 0, 0), param->face_frame_width, 8);

                    ASF_FaceFeature  feature_face;   // 存储人脸特征
                    ASF_SingleFaceInfo SingleDetectedFaces2;

                    SingleDetectedFaces2.faceRect.left = 0;
                    SingleDetectedFaces2.faceRect.top = 0;
                    SingleDetectedFaces2.faceRect.right = faceInfoTmp.matFaceROI.cols-1;
                    SingleDetectedFaces2.faceRect.bottom = faceInfoTmp.matFaceROI.rows-1;
                    SingleDetectedFaces2.faceOrient = faceInfoTmp.faceOrient;
                    m_arcsoft_res = ASFFaceFeatureExtract(m_arcsoft_handle, faceInfoTmp.matFaceROI.cols, faceInfoTmp.matFaceROI.rows, ASVL_PAF_RGB24_B8G8R8, (MUInt8*)faceInfoTmp.matFaceROI.data, &SingleDetectedFaces2, &feature_face);
                    if (MOK != m_arcsoft_res)
                    {
                        printf_log(string("ASFFaceFeatureExtract fail").data(), m_arcsoft_res);
                        continue;
                    }
                    //拷贝feature
                    FaceFeatureInfo info;
                    info.faceSampleFeature.featureSize = feature_face.featureSize;
                    info.faceSampleFeature.feature = (MByte *)malloc(feature_face.featureSize);
                    memset(info.faceSampleFeature.feature, 0, feature_face.featureSize);
                    memcpy(info.faceSampleFeature.feature, feature_face.feature, feature_face.featureSize);
                    m_tmpVecFace.push_back(info);
                }
            }
        }
    }
    for (int idx = 0; idx < m_tmpVecFace.size(); idx++)
    {
        SafeFree(m_tmpVecFace[idx].faceSampleFeature.feature);		//释放内存
    }

    return 0;
}

int CArcSoftFaceProc::face_detect_import()
{
    Mat frameROI;
    Rect rectFace;
    for(int count = 0; count < m_vecFaceInfoImport.size(); count++)
    {
        ASF_MultiFaceInfo detectedFaces1;

        m_vecFaceInfoImport[count].ori.copyTo(frameROI);  // 获取ROI区域

        // 进行人脸检测
        m_arcsoft_res = ASFDetectFaces(m_arcsoft_handle, frameROI.cols, frameROI.rows, ASVL_PAF_RGB24_B8G8R8, (MUInt8*)frameROI.data, &detectedFaces1);
        if (MOK != m_arcsoft_res)
        {
            printf_log(string("ASFDetectFaces fail").data(), m_arcsoft_res);
            return count;
        }

        if(detectedFaces1.faceNum != 1)
        {
            return count;
        }

        for (int i = 0; i < detectedFaces1.faceNum; i++)
        {
            rectFace.x = detectedFaces1.faceRect[i].left;
            rectFace.y = detectedFaces1.faceRect[i].top;
            rectFace.width = abs(detectedFaces1.faceRect[i].right - detectedFaces1.faceRect[i].left + 1);
            rectFace.height = abs(detectedFaces1.faceRect[i].bottom - detectedFaces1.faceRect[i].top + 1);
            /************ 参数检测：START **************/
            if (rectFace.x < 0)
                rectFace.x = 0;
            if (rectFace.y < 0)
                rectFace.y = 0;
            if (rectFace.br().x > frameROI.cols)
                rectFace.width = frameROI.cols - rectFace.x;
            if (rectFace.br().y > frameROI.rows)
                rectFace.height = frameROI.rows - rectFace.y;

            if (0 != rectFace.width % 4)
                rectFace.width = ((rectFace.width + 3) / 4) * 4;
            if (0 != rectFace.height % 2)
                rectFace.height = ((rectFace.height + 1) / 2) * 2;
            if (rectFace.br().x > frameROI.cols)
                rectFace.width = rectFace.width - 4;
            if (rectFace.br().y > frameROI.rows)
                rectFace.height = rectFace.height - 2;
            /************ 参数检测：END **************/
            m_vecFaceInfoImport[count].faceOrient = detectedFaces1.faceOrient[i]; // 人脸倾斜角度
            frameROI(rectFace).copyTo(m_vecFaceInfoImport[count].matFaceROI);

            //  提取人脸特征
            ASF_FaceFeature  feature_face;   // 存储人脸特征
            ASF_SingleFaceInfo SingleDetectedFaces2;

            FaceRoiInfoImport faceRoiTmp =m_vecFaceInfoImport[count];
            SingleDetectedFaces2.faceRect.left = 0;
            SingleDetectedFaces2.faceRect.top = 0;
            SingleDetectedFaces2.faceRect.right = faceRoiTmp.matFaceROI.cols-1;
            SingleDetectedFaces2.faceRect.bottom = faceRoiTmp.matFaceROI.rows-1;
            SingleDetectedFaces2.faceOrient = faceRoiTmp.faceOrient;
            m_arcsoft_res = ASFFaceFeatureExtract(m_arcsoft_handle, faceRoiTmp.matFaceROI.cols, faceRoiTmp.matFaceROI.rows, ASVL_PAF_RGB24_B8G8R8, (MUInt8*)faceRoiTmp.matFaceROI.data, &SingleDetectedFaces2, &feature_face);
            if (MOK != m_arcsoft_res)
            {
                printf_log(string("ASFFaceFeatureExtract fail").data(), m_arcsoft_res);
                return count;
            }
            //拷贝feature
            m_vecFaceInfoImport[count].faceFeature.featureSize = feature_face.featureSize;
            m_vecFaceInfoImport[count].faceFeature.feature = (MByte *)malloc(feature_face.featureSize);
            memset(m_vecFaceInfoImport[count].faceFeature.feature, 0, feature_face.featureSize);
            memcpy(m_vecFaceInfoImport[count].faceFeature.feature, feature_face.feature, feature_face.featureSize);
        }
    }

    return -1;
}

void CArcSoftFaceProc::clear_import_data()
{
    for (int idx = 0; idx < m_vecFaceInfoImport.size(); idx++)
    {
        SafeFree(m_vecFaceInfoImport[idx].faceFeature.feature);		//释放内存
    }
    m_vecFaceInfoImport.clear();
    m_vecFaceInfoImport.shrink_to_fit();
}

int CArcSoftFaceProc::face_recognition_import()
{
    if(m_vecFaceInfoImport.empty())
    {
        return -1;
    }
    for (int i = 0; i < m_vecFaceFeatureInfo.size(); i++)
    {
        // 单人脸特征比对
        vector<Student_Recognize> m_recognize_student;
        for(int j = 0; j < m_vecFaceInfoImport.size(); j++)
        {
            MFloat confidenceLevel = .0;;
            m_arcsoft_res = ASFFaceFeatureCompare(m_arcsoft_handle, &m_vecFaceFeatureInfo[i].faceSampleFeature, &m_vecFaceInfoImport[j].faceFeature, &confidenceLevel);
            if (MOK != m_arcsoft_res)
            {
                printf_log(string("ASFFaceFeatureCompare fail").data(), m_arcsoft_res);
            }
            Student_Recognize student;
            student.index = j;
            student.confidence = confidenceLevel;
            student.name = m_vecFaceInfoImport[j].name;
            student.indexOf = j;
            m_recognize_student.push_back(student);
        }
        sort(m_recognize_student.begin(), m_recognize_student.end(), compare);
        for(int j = 0; j < MIN(m_recognize_student.size(), 3); j++)
        {
            m_vecFaceFeatureInfo[i].Student[j].count = m_recognize_student[j].index;
            m_vecFaceFeatureInfo[i].Student[j].confidence = m_recognize_student[j].confidence;
            m_vecFaceFeatureInfo[i].Student[j].name = m_recognize_student[j].name;
            m_vecFaceFeatureInfo[i].Student[j].indexOf = m_recognize_student[j].indexOf;
        }
    }
    return 0;
}

int CArcSoftFaceProc::face_video_import(const char *video_path, VideoDecter video_detect)
{
    m_video_detecter = video_detect;
    cap.open(video_path);
    m_totalFrameNum = cap.get(CV_CAP_PROP_FRAME_COUNT);
    m_currentFrameNum = 1;
    cap.set(CV_CAP_PROP_POS_FRAMES, m_currentFrameNum);
    if(m_totalFrameNum > 1 && cap.isOpened())
    {
        m_tx.lock();
        while(cap.isOpened())
        {

            cap>>m_current_frame;
            if(!m_current_frame.empty())
                break;
        }
        m_tx.unlock();
        std::thread t(move_face_detect_to_thread, this);
        t.detach();
        thread_guard g(t);
    }
    else
        return -1;
    return 0;
}

int CArcSoftFaceProc::split(string src, vector<string> &dest, char delim)
{
    int blankPos = src.find_first_of(delim);
    string timfStr = src.substr(0,blankPos);
    string contentTmp = src.substr(blankPos + 1);
    dest.push_back(timfStr);
    dest.push_back(contentTmp);
    return 0;
}

//从大到小排序
bool CArcSoftFaceProc::compare(const Student_Recognize &a, const Student_Recognize &b)
{
    return a.confidence > b.confidence;
}

void CArcSoftFaceProc::clearAll()
{
    m_tx.lock();
    detected_faceinfo_clear();
    faceset_clear();
    clear_import_data();
    m_tx.unlock();
}

void CArcSoftFaceProc::setImportVecSize(int size)
{
    for(int i = 0; i < size; i ++)
    {
        FaceRoiInfoImport info;
        m_vecFaceInfoImport.push_back(info);
    }
}

void CArcSoftFaceProc::getCurrentFrame(Mat &frame)
{
    m_ori_detect_img.release();
    m_current_frame.copyTo(m_ori_detect_img);
    m_current_frame.copyTo(frame);
}

void CArcSoftFaceProc::move_face_detect_to_thread(void *arg)
{
    CArcSoftFaceProc *io = (CArcSoftFaceProc *)arg;
    io->m_tx.lock();
    double rate = io->cap.get(CV_CAP_PROP_FPS);
    long current_frame_num = io->cap.get(CV_CAP_PROP_POS_FRAMES);
    long frame_count = io->cap.get(CV_CAP_PROP_FRAME_COUNT);
    io->m_tx.unlock();
    bool opened = true;
    while(opened && current_frame_num < frame_count)
    {
        io->m_tx.lock();
        io->cap>>io->m_current_frame;
        opened = io->cap.isOpened();
        io->m_tx.unlock();
        Sleep(1000 / rate);
        current_frame_num++;
    }
    io->m_tx.lock();
    io->cap.release();
    io->m_tx.unlock();
}
