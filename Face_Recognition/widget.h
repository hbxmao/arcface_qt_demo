#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QRegExpValidator>
#include <QFileDialog>
#include <QMessageBox>
#include <opencv2/opencv.hpp>
#include <QDebug>
#include <QScrollArea>
#include <QTimer>
#include <QThread>
#include <QList>
#include <QPainter>
#include <QTime>

#include "facedefine.h"
#include "arcsoft_face_proc.h"

#include "TLEduCameraMR.h"
#include "TLEduMRCameraReciver.h"

#include "cvxtext.h"

#define SafeFree(p) { if ((p)) free(p); (p) = NULL; }

namespace Ui {
class Widget;
}

typedef struct __result_name
{
    Rect    rect;
    string  name;
}Result_Name;

class CamraRecv : public CTLEduMRCameraReciver
{
public:
    CamraRecv();
    virtual ~CamraRecv();
    //相机收到数据的回调函数
    //bVideo为1表示视频流，0表示音频流；
    //bEs为1表示压缩码流，0表示解码码流
    virtual void OnGotFrame(BYTE* pBuf, int buf_len, LONGLONG pts, BOOL bVideo, BOOL bEs);

};

class FaceDetectThread : public QObject
{
    Q_OBJECT

public:
    explicit FaceDetectThread(QWidget *parent = nullptr);
    ~FaceDetectThread();

public slots:
    void start_face_detect(Mat &frame);

signals:
    void finish_face_detect(Mat &frame);
    void spend_time(QString);
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();

public:
    QTimer *m_timer;        //获取视频帧的计时器
    QThread *m_thread;      //人脸识别线程
    FaceDetectThread *m_face;
    Mat show_big_image;
    QList<Result_Name> m_result_name;

    string m_lineEditIp = "192.168.5.163";
    CTLEduCameraMR *Cipc_T = nullptr;
    TLEduMRCameraParam *param_T;
    CamraRecv *recv_T;

public:
    void init();
    //每50帧调用一次函数指针
    static void video_face_detect(Mat res);
    //将结果显示出来
    void create_face_recognition_result();
    //右侧列表显示最终结果
    void show_face_recogintion_result(int select = 0);
    //显示识别出同学的姓名
    void show_face_recognition_name(Mat &image);

private slots:
    //显示耗时
    void show_spend_time(QString);
    //显示最终处理结果
    void show_video_face_detect(Mat &res);

    void getcurrentframe();

    void on_close_clicked();

    void on_connect_clicked();

    void on_start_clicked();

    void on_stop_clicked();

    void on_open_clicked();

    void on_import_btn_clicked();

    void on_para_hide_clicked();

    void on_res_clicked();

    void on_save_clicked();

    void on_select_stateChanged(int arg1);

    void on_move_clicked();

    void on_lineEdit_textChanged(const QString &arg1);

    void on_camera_clicked();

    void on_image_clicked();

    void on_video_clicked();

    void on_leftup_clicked();

    void on_up_clicked();

    void on_rightup_clicked();

    void on_left_clicked();

    void on_home_clicked();

    void on_right_clicked();

    void on_leftdown_clicked();

    void on_down_clicked();

    void on_rightdown_clicked();

    void on_horizontalSlider_valueChanged(int value);

    void on_zoomadd_clicked();

    void on_zoomdec_clicked();

    void on_foucsadd_clicked();

    void on_focusdec_clicked();

    void on_irisadd_clicked();

    void on_irisdec_clicked();

    void on_comboBox_currentIndexChanged(int index);

    void on_set_clicked();

    void on_delete_2_clicked();

    void on_call_clicked();

    void on_checkBox_clicked(bool checked);

signals:
    void start_face_detect(Mat&);

private:
    Ui::Widget *ui;

    static Widget *m_self;
};

#endif // WIDGET_H
