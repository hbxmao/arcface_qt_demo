#include "widget.h"
#include "ui_widget.h"

#if _MSC_VER >=1600 //VS2010版本号是1600
#pragma execution_character_set("utf-8")
#endif

FILE *fp = NULL;

Widget *Widget::m_self = nullptr;
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    init();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::init()
{
    this->setWindowTitle("人脸检测测试软件");
    setWindowFlags(Qt::WindowCloseButtonHint);

    m_self = this;

    //初始化虹软界面
    ui->tabWidget->setCurrentIndex(0);
    ui->tabWidget_2->setCurrentIndex(0);

    QRegExp rx("((2[0-4]\\d|25[0-5]|[01]?\\d\\d?)\\.){3}(2[0-4]\\d|25[0-5]|[01]?\\d\\d?)");
    QRegExpValidator v(rx, nullptr);
    ui->lineEdit->setValidator(&v);

    //初始化虹软算法引擎
    m_arcSoftFaceProc.face_proc_init();

    ui->scrollArea->hide();
    //读取参数配置文件
    ifstream in;
    in.open("./param", ios::in | ios::binary);
    if(in.is_open())
    {
        m_faceRecognition_setting = (unsigned char *)malloc(sizeof (FaceRecognitionSetting));
        in.read(reinterpret_cast<char*>(m_faceRecognition_setting), sizeof (FaceRecognitionSetting));
        FaceRecognitionSetting *param = (FaceRecognitionSetting *)m_faceRecognition_setting;
        if(param->layer == 4)
            ui->layer->setCurrentIndex(0);
        else if(param->layer == 6)
            ui->layer->setCurrentIndex(1);
        if(param->recognize == 4)
            ui->recognize_threshold->setCurrentIndex(0);
        else if(param->recognize == 5)
            ui->recognize_threshold->setCurrentIndex(1);
        else if(param->recognize == 6)
            ui->recognize_threshold->setCurrentIndex(2);
        ui->div_deno->setCurrentIndex(param->div_ori_deno - 1);
        ui->camera_scan_mode->setCurrentIndex(param->camera_scan_mode);
        ui->face_width->setCurrentIndex(param->face_frame_width - 1);
        ui->face_display_num->setCurrentIndex(param->face_display_num / 50 - 1);
        ui->time_interval->setCurrentIndex(param->time_interval - 1);
        in.close();
    }
    else
    {
        ofstream out;
        out.open("./param", ios::out | ios::binary);
        m_faceRecognition_setting = (unsigned char *)malloc(sizeof (FaceRecognitionSetting));
        FaceRecognitionSetting *param = (FaceRecognitionSetting *)m_faceRecognition_setting;
        param->layer = ui->layer->currentText().toInt();
        param->recognize = ui->recognize_threshold->currentText().toInt();
        param->div_ori_deno = ui->div_deno->currentText().toInt();
        param->camera_scan_mode = ui->camera_scan_mode->currentIndex();
        param->face_frame_width = ui->face_width->currentText().toInt();
        param->face_display_num = ui->face_display_num->currentText().toInt();
        param->time_interval = ui->time_interval->currentText().toInt();
        out<<(char *)m_faceRecognition_setting;
        out.close();
    }

    qRegisterMetaType<cv::Mat>("Mat &");

    m_thread = new QThread;
    m_face = new FaceDetectThread(this);
    m_face->moveToThread(m_thread);
    m_thread->start();
    connect(this, SIGNAL(start_face_detect(Mat&)), m_face, SLOT(start_face_detect(Mat &)));
    connect(m_face, SIGNAL(finish_face_detect(Mat &)), this, SLOT(show_video_face_detect(Mat &)));
    connect(m_face, SIGNAL(spend_time(QString)), this, SLOT(show_spend_time(QString)));

    m_timer = new QTimer(this);
    m_timer->setInterval(ui->time_interval->currentText().toInt() * 1000);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(getcurrentframe()));
}

void Widget::create_face_recognition_result()
{
    if(ui->tabWidget->currentIndex() == 0)
    {
        vector<FaceFeatureInfo> &m_vecFaceFeatureInfo = m_arcSoftFaceProc.m_vecFaceFeatureInfo;
        vector<FaceRoiInfo> &m_vecFaceInfo = m_arcSoftFaceProc.m_vecFaceInfo;
        vector<FaceRoiInfoImport> &m_vecFaceInfoImport = m_arcSoftFaceProc.m_vecFaceInfoImport;
        int detect_face_num = m_vecFaceInfo.size();
        int rowCount = ui->arcsoft->rowCount();
        ui->arcsoft->setRowCount(rowCount + detect_face_num);
        for(int i = rowCount; i < ui->arcsoft->rowCount(); i++)
        {
            bool recognize = m_vecFaceInfo[i - rowCount].recognize;
            QLabel *label = new QLabel(ui->arcsoft);
            Mat tmp;
            m_vecFaceInfo[i - rowCount].matFaceROI.copyTo(tmp);
            cvtColor(tmp, tmp, CV_BGR2RGB);
            QImage face = QImage(tmp.data, tmp.cols, tmp.rows, QImage::Format_RGB888);
            label->setPixmap(QPixmap::fromImage(face).scaled(SHOW_FACE_WIDTH, SHOW_FACE_WIDTH));
            ui->arcsoft->setCellWidget(i, 0, label);
            if(recognize)
            {
                Mat tmp;
                QLabel *label = new QLabel(ui->arcsoft);
                m_vecFaceInfoImport[m_vecFaceFeatureInfo[i - rowCount].Student[0].indexOf].ori.copyTo(tmp);
                cvtColor(tmp, tmp, CV_BGR2RGB);
                QImage ori = QImage(tmp.data, tmp.cols, tmp.rows, QImage::Format_RGB888);
                label->setPixmap(QPixmap::fromImage(ori).scaled(SHOW_FACE_WIDTH, SHOW_FACE_WIDTH));
                ui->arcsoft->setCellWidget(i, 1, label);

                int index = 0;
                QString data = QString();
                while(index < 3)
                {
                    if(m_vecFaceFeatureInfo[i - rowCount].Student[index].confidence != 0)
                    {
                        data += QString::fromUtf8("姓名：");
                        data += QString(QString::fromLocal8Bit(m_vecFaceFeatureInfo[i - rowCount].Student[index].name.data()));
                        data += "\n";
                        data += QString(tr("置信度："));
                        data += QString::number(m_vecFaceFeatureInfo[i - rowCount].Student[index].confidence) + "\n";
                    }
                    index++;
                }
                QLabel *label2 = new QLabel(ui->arcsoft);
                label2->setText(data);
                ui->arcsoft->setCellWidget(i, 2, label2);
            }
            else
            {
                QLabel *label = new QLabel(ui->arcsoft);
                label->setText(QString::fromUtf8("人脸信息提取失败"));
                ui->arcsoft->setCellWidget(i, 2, label);
            }
        }
        rowCount = ui->arcsoft->rowCount();
        FaceRecognitionSetting *param = (FaceRecognitionSetting *)m_faceRecognition_setting;
        if(rowCount > param->face_display_num)
        {
            int num = rowCount - param->face_display_num;
            for(int i = 0; i < num; i++)
            {
                ui->arcsoft->removeRow(0);
            }
        }
        ui->arcsoft->scrollToBottom();
    }
}

void Widget::show_face_recogintion_result(int select)
{
    vector<FaceFeatureInfo> &m_vecFaceFeatureInfo = m_arcSoftFaceProc.m_vecFaceFeatureInfo;
    vector<FaceRoiInfo> &m_vecFaceInfo = m_arcSoftFaceProc.m_vecFaceInfo;
    vector<FaceRoiInfoImport> &m_vecFaceInfoImport = m_arcSoftFaceProc.m_vecFaceInfoImport;
    FaceRecognitionSetting *param = (FaceRecognitionSetting *)m_faceRecognition_setting;
    for(int i = 0; i < m_vecFaceInfo.size(); i++)
    {
        bool recognize = m_vecFaceInfo[i].recognize;
        string name = m_vecFaceFeatureInfo[i].Student[0].name;
        if(recognize && m_vecFaceFeatureInfo[i].Student[0].confidence > (float)param->recognize / 10)
        {
            string same_name = "";
            float confidence = m_vecFaceFeatureInfo[i].Student[0].confidence;
            int index = -1;
            int rowcount = ui->result->rowCount();
            for(int j = 0; j < rowcount;j++)
            {
                QLabel *item = (QLabel*)ui->result->cellWidget(j, 2);
                QString text = item->text();
                string name = text.split("\n")[0].split(":")[1].toLocal8Bit().data();
                if(m_vecFaceFeatureInfo[i].Student[0].name.compare(name) == 0)
                {
                    same_name = m_vecFaceFeatureInfo[i].Student[0].name;
                    if(confidence > text.split("\n")[1].split(":")[1].toFloat())
                    {
                        index = j;
                    }
                }
            }
            if(select == 0 || same_name.empty())
            {
                bool same = false;
                for(int k = 0; k < m_result_name.size(); k++)
                {
                    if(m_result_name[k].name.compare(m_vecFaceFeatureInfo[i].Student[0].name) == 0)
                    {
                        same = true;
                        break;
                    }
                }
                if(m_result_name.size() == 0 || same == false)
                {
                    Result_Name result_name;
                    result_name.rect = m_vecFaceFeatureInfo[i].rect;
                    result_name.name = m_vecFaceFeatureInfo[i].Student[0].name;
                    m_result_name.push_back(result_name);
                }

                ui->result->setRowCount(ui->result->rowCount() + 1);
                {
                    QLabel *label = new QLabel(ui->result);
                    Mat tmp;
                    m_vecFaceInfo[i].matFaceROI.copyTo(tmp);
                    cvtColor(tmp, tmp, CV_BGR2RGB);
                    QImage face = QImage(tmp.data, tmp.cols, tmp.rows, QImage::Format_RGB888);
                    label->setPixmap(QPixmap::fromImage(face).scaled(SHOW_FACE_WIDTH, SHOW_FACE_WIDTH));
                    ui->result->setCellWidget(ui->result->rowCount() - 1, 0, label);
                }

                {
                    Mat tmp;
                    QLabel *label = new QLabel(ui->result);
                    m_vecFaceInfoImport[m_vecFaceFeatureInfo[i].Student[0].indexOf].ori.copyTo(tmp);
                    cvtColor(tmp, tmp, CV_BGR2RGB);
                    QImage ori = QImage(tmp.data, tmp.cols, tmp.rows, QImage::Format_RGB888);
                    label->setPixmap(QPixmap::fromImage(ori).scaled(SHOW_FACE_WIDTH, SHOW_FACE_WIDTH));
                    ui->result->setCellWidget(ui->result->rowCount() - 1, 1, label);
                }

                int index = 0;
                QString data = QString();
                while(index < 3)
                {
                    if(m_vecFaceFeatureInfo[i].Student[index].confidence != 0)
                    {
                        data += QString::fromUtf8("姓名:");
                        data += QString(QString::fromLocal8Bit(m_vecFaceFeatureInfo[i].Student[index].name.data()));
                        data += "\n";
                        data += QString(tr("置信度:"));
                        data += QString::number(m_vecFaceFeatureInfo[i].Student[index].confidence) + "\n";
                    }
                    index++;
                }
                QLabel *label2 = new QLabel(ui->result);
                label2->setText(data);
                ui->result->setCellWidget(ui->result->rowCount() - 1, 2, label2);
            }
            else if(index != -1)
            {
                {
                    QLabel *label = new QLabel(ui->result);
                    Mat tmp;
                    m_vecFaceInfo[i].matFaceROI.copyTo(tmp);
                    cvtColor(tmp, tmp, CV_BGR2RGB);
                    QImage face = QImage(tmp.data, tmp.cols, tmp.rows, QImage::Format_RGB888);
                    label->setPixmap(QPixmap::fromImage(face).scaled(SHOW_FACE_WIDTH, SHOW_FACE_WIDTH));
                    ui->result->setCellWidget(index, 0, label);
                }
                {
                    Mat tmp;
                    QLabel *label = new QLabel(ui->result);
                    m_vecFaceInfoImport[m_vecFaceFeatureInfo[i].Student[0].indexOf].ori.copyTo(tmp);
                    cvtColor(tmp, tmp, CV_BGR2RGB);
                    QImage ori = QImage(tmp.data, tmp.cols, tmp.rows, QImage::Format_RGB888);
                    label->setPixmap(QPixmap::fromImage(ori).scaled(SHOW_FACE_WIDTH, SHOW_FACE_WIDTH));
                    ui->result->setCellWidget(index, 1, label);
                }

                int count = 0;
                QString data = QString();
                while(count < 3)
                {
                    if(m_vecFaceFeatureInfo[i].Student[count].confidence != 0)
                    {
                        data += QString::fromUtf8("姓名:");
                        data += QString(QString::fromLocal8Bit(m_vecFaceFeatureInfo[i].Student[count].name.data()));
                        data += "\n";
                        data += QString(tr("置信度:"));
                        data += QString::number(m_vecFaceFeatureInfo[i].Student[count].confidence) + "\n";
                    }
                    count++;
                }
                QLabel *label2 = new QLabel(ui->result);
                label2->setText(data);
                ui->result->setCellWidget(index, 2, label2);
            }
        }
    }
    int rowCount = ui->result->rowCount();
    if(rowCount > param->face_display_num)
    {
        int num = rowCount - param->face_display_num;
        for(int i = 0; i < num; i++)
        {
            ui->result->removeRow(0);
        }
    }
    ui->result->scrollToBottom();
}

void Widget::show_face_recognition_name(Mat &image)
{
    CvxText text("./msyhbd.ttc");
    float p = 0.5;
    CvScalar size(36, 0.5, 0.1, 0);
    text.setFont(NULL, &size, NULL, &p);
    for(int i = 0; i < m_result_name.size(); i++)
    {
        text.putText(image, m_result_name[i].name.data(), CvPoint(m_result_name[i].rect.x, m_result_name[i].rect.y), CV_RGB(0,0,255));
    }
}

void Widget::show_spend_time(QString time)
{
    ui->spend_time->setText(time);
}

void Widget::getcurrentframe()
{
    m_arcSoftFaceProc.m_tx.lock();
    if(!m_arcSoftFaceProc.cap.isOpened())
    {
        m_timer->stop();
        m_arcSoftFaceProc.m_tx.unlock();
        return;
    }
    m_arcSoftFaceProc.faceset_clear();
    m_arcSoftFaceProc.detected_faceinfo_clear();
    m_result_name.clear();
    m_arcSoftFaceProc.m_tx.unlock();
    Mat frame;
    m_arcSoftFaceProc.m_tx.lock();
    m_arcSoftFaceProc.getCurrentFrame(frame);
    m_arcSoftFaceProc.m_tx.unlock();
    emit start_face_detect(frame);
}

void Widget::video_face_detect(Mat res)
{
    m_self->show_video_face_detect(res);
}

void Widget::show_video_face_detect(Mat &res)
{
    if(res.empty())
        return;
    m_arcSoftFaceProc.m_tx.lock();
    create_face_recognition_result();
    show_face_recogintion_result(ui->select->isChecked());
    show_face_recognition_name(res);
    m_arcSoftFaceProc.m_tx.unlock();
    show_big_image.release();
    res.copyTo(show_big_image);
    cvtColor(res, res, CV_BGR2RGB);
    QImage image = QImage(res.data, res.cols, res.rows, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image).scaled(ORI_Width, ORI_HEIGHT);
    ui->move->setIcon(QIcon(pixmap));
//    ui->move->setPixmap(QPixmap::fromImage(image).scaled(ORI_Width, ORI_HEIGHT));
}

void Widget::on_close_clicked()
{
    this->close();
}


void Widget::on_connect_clicked()
{
    ui->camera->setChecked(true);

    param_T = new TLEduMRCameraParam;         // 相机参数结构体
    recv_T = new CamraRecv;    // 相机取流回调类

    fp = fopen("D:\\MRPCMTest.pcm", "wb+");
    if (fp == NULL)
    {
        return;
    }

    param_T->m_in_video_type = 0;
    param_T->m_work_mod = 1; //相机的工作模式，0：流拷贝模式，1：解码模式，2：混合模式，同时输出压缩和无压缩数据
    param_T->m_in_video_height = 1080;
    param_T->m_in_video_width = 1920;
    param_T->m_pReciver = recv_T;

    strcpy(param_T->s_ipc_IP, m_lineEditIp.data());  // IP地址
    strcpy(param_T->s_ipc_UserName, "admin");    // 用户名
    strcpy(param_T->s_ipc_Password, "admin");    // 密码

    Cipc_T = CTLEduCameraMR::CreateObj();
    bool connect = Cipc_T->InitObj(param_T);    // 相机类初始化
    if(!connect)
    {
        on_stop_clicked();
        QMessageBox::information(this, tr("相机初始化"), QString("相机初始化失败！"));
        return;
    }
}

void Widget::on_start_clicked()
{
    int flag_T = Cipc_T->Start(); // 开启码流
    if (flag_T)
        cout << "I: Techer IPC Success " << endl;
    else
        cout << "I: Techer IPC Fail " << endl;
    m_timer->start();
}

void Widget::on_stop_clicked()
{
    if(Cipc_T != nullptr)
    {
        m_timer->stop();

        Cipc_T->Stop();
        Cipc_T->ReleaseObj();
        CTLEduCameraMR::ReleaseObj(Cipc_T);

        fclose(fp);
        fp = NULL;
    }
    if(m_arcSoftFaceProc.cap.isOpened())
    {
        m_timer->stop();
        m_arcSoftFaceProc.cap.release();
    }
}

void Widget::on_open_clicked()
{
    QString filename = QString();
    if(ui->image->isChecked())
        filename = QFileDialog::getOpenFileName(ui->groupBox, tr("打开图片"), tr("/"), tr("Image File(*.png *.jpg *.bmp)"));
    else if(ui->video->isChecked())
        filename = QFileDialog::getOpenFileName(ui->groupBox, tr("打开视频"), tr("/"), tr("File(*)"));
    if(!filename.isEmpty())
    {
        //虹软算法
        if(ui->tabWidget->currentIndex() == 0)
        {
            Mat img = imread(filename.toLocal8Bit().data());
            if(!img.empty())
            {
                Mat res;
                ui->arcsoft->clearContents();
                ui->arcsoft->setRowCount(0);
                ui->result->clearContents();
                ui->result->setRowCount(0);
                ui->ori->clear();
//                ui->res->clear();
                ui->res->setIcon(QIcon());
                m_arcSoftFaceProc.m_tx.lock();
                m_result_name.clear();
                m_arcSoftFaceProc.detected_faceinfo_clear();
                m_arcSoftFaceProc.faceset_clear();
                m_arcSoftFaceProc.m_tx.unlock();
                int ret = m_arcSoftFaceProc.face_image_import(filename.toLocal8Bit().data(), res);
                create_face_recognition_result();
                show_face_recogintion_result(ui->select->isChecked());
                if (ret >= 0)
                {
                    ui->ori->setPixmap(QPixmap(filename).scaled(ORI_Width, ORI_HEIGHT));
                    show_face_recognition_name(res);
                    show_big_image.release();
                    res.copyTo(show_big_image);
                    cvtColor(res, res, CV_BGR2RGB);
                    QImage image = QImage(res.data, res.cols, res.rows, QImage::Format_RGB888);
                    ui->res->setIcon(QIcon(QPixmap::fromImage(image).scaled(ORI_Width, ORI_HEIGHT)));
                }
                else
                {
                    QMessageBox::warning(this, tr("学生信息导入"), tr("请导入学生信息！！！"));
                    return;
                }
            }
            //视频
            else
            {
                ui->arcsoft->clearContents();
                ui->arcsoft->setRowCount(0);
                ui->result->clearContents();
                ui->result->setRowCount(0);
                m_arcSoftFaceProc.m_tx.lock();
                m_arcSoftFaceProc.detected_faceinfo_clear();
                m_arcSoftFaceProc.faceset_clear();
                m_arcSoftFaceProc.m_tx.unlock();
                VideoDecter video_face = video_face_detect;
                int ret = m_arcSoftFaceProc.face_video_import(filename.toLocal8Bit().data(), video_face);
                if (ret < 0)
                {
                    QMessageBox::warning(this, tr("视频导入"), tr("导入视频失败！！！"));
                    return;
                }
                m_timer->start();
            }
        }
        //其他算法
        else
        {
            //
        }
    }
}

void Widget::on_import_btn_clicked()
{
#if 0
    QStringList list = QFileDialog::getOpenFileNames(ui->groupBox, tr("导入文件"), tr("/"), tr("File(*.png *.jpg *.bmp *.txt)"));
    if(!list.isEmpty())
    {
        m_arcSoftFaceProc.clearAll();
        m_arcSoftFaceProc.setImportVecSize(list.size() - 1);
        int index = 0;
        for(int i = 0; i < list.size(); i++)
        {
            if(list[i].endsWith(".txt"))
            {
                m_arcSoftFaceProc.face_image_import_set_name(list[i].toLocal8Bit().data());
            }
            else
            {
                int ret = m_arcSoftFaceProc.face_image_import_origin(list[i].toLocal8Bit().data(), index);
                if(ret < -1)
                {
                    QMessageBox::warning(this, tr("导入图片"), "发现空图片，请立即替换正确图片！\n终止导入！！！");
                    m_arcSoftFaceProc.clear_import_data();
                    break;
                }
                else if(ret != -1)
                {
                    QMessageBox::warning(this, tr("导入图片"), QString("%1号同学证件照尺寸不正确，请立即替换正常尺寸图片\n终止导入！！！").arg(ret));
                    m_arcSoftFaceProc.clear_import_data();
                    break;
                }
                index++;
            }
        }
        int ret = m_arcSoftFaceProc.face_detect_import();
        if(ret >= 0)
        {
            QMessageBox::warning(this, tr("导入图片"), QString("%1号同学证件照没有人脸信息或含有多张人脸信息！").arg(ret));
            m_arcSoftFaceProc.clear_import_data();
            return;
        }
        QMessageBox::information(this, tr("导入图片"), QString("学生信息导入成功！"));
    }
#else
    QString path = QFileDialog::getExistingDirectory(ui->groupBox, tr("导入文件"), tr("/"));
    QDir dir(path);
    dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    const QFileInfoList files = dir.entryInfoList();
    if(!files.empty() && !path.isEmpty())
    {
        m_arcSoftFaceProc.clearAll();
        m_arcSoftFaceProc.setImportVecSize(files.size() - 1);
        for(int i = 0; i < files.size(); i++)
        {
            QFileInfo info = files[i];
            if(info.isDir())
            {
                int index = info.fileName().toInt();
                QString subdir = info.absoluteFilePath();
                QDir dir(subdir);
                dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
                const QFileInfoList list = dir.entryInfoList();
                for(int i = 0; i < list.size(); i++)
                {
                    int ret = m_arcSoftFaceProc.face_image_import_origin(list[i].absoluteFilePath().toLocal8Bit().data(), index);
                    if(ret < -1)
                    {
                        QMessageBox::warning(this, tr("导入图片"), "发现空图片，请立即替换正确图片！\n终止导入！！！");
                        break;
                    }
                    else if(ret != -1)
                    {
                        QMessageBox::warning(this, tr("导入图片"), QString("%1号同学证件照尺寸不正确，请立即替换正常尺寸图片\n终止导入！！！").arg(ret));
                        break;
                    }
                }
            }
            else if(info.fileName().endsWith(".txt"))
            {
                m_arcSoftFaceProc.face_image_import_set_name(info.absoluteFilePath().toLocal8Bit().data());
            }
        }
        int ret = m_arcSoftFaceProc.face_detect_import();
        if(ret >= 0)
        {
            QMessageBox::warning(this, tr("导入图片"), QString("%1号同学证件照没有人脸信息或含有多张人脸信息！").arg(ret));
            return;
        }
        QMessageBox::information(this, tr("导入图片"), QString("学生信息导入成功！"));
#endif
    }
}

void Widget::on_para_hide_clicked()
{
    ui->scrollArea->hide();
}

void Widget::on_save_clicked()
{
    //读取参数配置文件
    ofstream ouf;
    ouf.open("./param", ios::out | ios::binary);
    if(ouf.is_open())
    {
        FaceRecognitionSetting *param = (FaceRecognitionSetting *)m_faceRecognition_setting;
        param->layer = ui->layer->currentText().toInt();
        param->recognize = ui->recognize_threshold->currentText().toInt();
        param->div_ori_deno = ui->div_deno->currentText().toInt();
        param->face_frame_width = ui->face_width->currentText().toInt();
        param->camera_scan_mode = ui->camera_scan_mode->currentIndex();
        param->face_display_num = ui->face_display_num->currentText().toInt();
        param->time_interval = ui->time_interval->currentText().toInt();
        ouf.write(reinterpret_cast<const char*>(m_faceRecognition_setting), sizeof (FaceRecognitionSetting));
        ouf.close();
    }
}

void Widget::on_select_stateChanged(int arg1)
{
    ui->result->clearContents();
    ui->result->setRowCount(0);
    show_face_recogintion_result(arg1);
}

FaceDetectThread::FaceDetectThread(QWidget *parent)
{

}

FaceDetectThread::~FaceDetectThread()
{

}

void FaceDetectThread::start_face_detect(Mat &frame)
{
    QTime time;
    time.start();
    m_arcSoftFaceProc.m_tx.lock();
    m_arcSoftFaceProc.face_detect(frame);
    m_arcSoftFaceProc.face_recognition_import();
    m_arcSoftFaceProc.m_tx.unlock();
    int sec = time.elapsed();
    QString data = "耗时：" + QString::number(sec) + " 毫秒";
    emit spend_time(data);
    emit finish_face_detect(frame);
}

void Widget::on_res_clicked()
{
    if(show_big_image.empty())
        return;
    namedWindow("Result", CV_WINDOW_NORMAL);
    imshow("Result", show_big_image);
}

void Widget::on_move_clicked()
{
    if(show_big_image.empty())
        return;
    namedWindow("Result", CV_WINDOW_NORMAL);
    imshow("Result", show_big_image);
}

//相机收到数据的回调函数
//bVideo为1表示视频流，0表示音频流
//bEs为1表示压缩码流，0表示解码码流
CamraRecv::CamraRecv()
{

}

CamraRecv::~CamraRecv()
{

}

void CamraRecv::OnGotFrame(BYTE *pBuf, int buf_len, LONGLONG pts, BOOL bVideo, BOOL bEs)
{
    int nHeight = 1080;
    int nWidth = 1920;
//    Mat matY = cv::Mat::zeros(nHeight, nWidth, CV_8UC1);

    //视频
    if (bVideo && (!bEs))
    {
//        Mat temp;
//        for (int i = 0; i < nHeight; i++)
//        {
//            uchar *tmp = matY.ptr<uchar>(i);
//            memcpy(tmp, pBuf + i*nWidth, sizeof(uchar)*nWidth);
//        }
        Mat dst(nHeight,nWidth,CV_8UC3);
        Mat src(nHeight + nHeight/2,nWidth,CV_8UC1,pBuf);
        cvtColor(src,dst,CV_YUV2BGR_I420);
        m_arcSoftFaceProc.m_tx.lock();
        m_arcSoftFaceProc.m_current_frame.release();
        dst.copyTo(m_arcSoftFaceProc.m_current_frame);
        m_arcSoftFaceProc.m_tx.unlock();
//        namedWindow("OutResult", 0);
//        imshow("OutResult", matY);
    }

    //音频
    /*
    if ((!bVideo) && (!bEs))
    {
        cout << "pts = " << pts << "  buf_len = " << buf_len << endl;
        int write_length = 0;
        write_length = fwrite(pBuf + buf_len / 2, 1, buf_len/2, fp);
        printf("write_length = %d\n", write_length);
    }
    */
}

void Widget::on_lineEdit_textChanged(const QString &arg1)
{
    m_lineEditIp = arg1.toStdString();
}

void Widget::on_camera_clicked()
{
    on_stop_clicked();
    ui->arcsoft->clearContents();
    ui->arcsoft->setRowCount(0);
    ui->result->clearContents();
    ui->result->setRowCount(0);
    m_arcSoftFaceProc.m_tx.lock();
    m_arcSoftFaceProc.faceset_clear();
    m_arcSoftFaceProc.detected_faceinfo_clear();
    m_arcSoftFaceProc.m_tx.unlock();
}

void Widget::on_image_clicked()
{
    ui->arcsoft->clearContents();
    ui->arcsoft->setRowCount(0);
    ui->result->clearContents();
    ui->result->setRowCount(0);
    m_arcSoftFaceProc.m_tx.lock();
    m_arcSoftFaceProc.faceset_clear();
    m_arcSoftFaceProc.detected_faceinfo_clear();
    m_arcSoftFaceProc.m_tx.unlock();
}

void Widget::on_video_clicked()
{
    on_stop_clicked();
    ui->arcsoft->clearContents();
    ui->arcsoft->setRowCount(0);
    ui->result->clearContents();
    ui->result->setRowCount(0);
    m_arcSoftFaceProc.m_tx.lock();
    m_arcSoftFaceProc.faceset_clear();
    m_arcSoftFaceProc.detected_faceinfo_clear();
    m_arcSoftFaceProc.m_tx.unlock();
}

void Widget::on_leftup_clicked()
{

}

void Widget::on_up_clicked()
{

}

void Widget::on_rightup_clicked()
{

}

void Widget::on_left_clicked()
{

}

void Widget::on_home_clicked()
{

}

void Widget::on_right_clicked()
{

}

void Widget::on_leftdown_clicked()
{

}

void Widget::on_down_clicked()
{

}

void Widget::on_rightdown_clicked()
{

}

void Widget::on_horizontalSlider_valueChanged(int value)
{

}

void Widget::on_zoomadd_clicked()
{

}

void Widget::on_zoomdec_clicked()
{

}

void Widget::on_foucsadd_clicked()
{

}

void Widget::on_focusdec_clicked()
{

}

void Widget::on_irisadd_clicked()
{

}

void Widget::on_irisdec_clicked()
{

}

void Widget::on_comboBox_currentIndexChanged(int index)
{

}

void Widget::on_set_clicked()
{

}

void Widget::on_delete_2_clicked()
{

}

void Widget::on_call_clicked()
{

}

void Widget::on_checkBox_clicked(bool checked)
{

}
