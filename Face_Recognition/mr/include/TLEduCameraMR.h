#pragma once
#include <windows.h>

#ifdef DLL_EXPORTS  
#define DLL_API __declspec(dllexport)  
#else  
#define DLL_API __declspec(dllimport)  
#endif  

class CTLEduMRCameraReciver;

//相机初始化参数
typedef struct TLEduMRCameraParam
{
	//相机的工作模式，0：流拷贝模式，1：解码模式，2：混合模式，同时输出压缩和无压缩数据
	int m_work_mod;

	//相机参数设置
	char s_ipc_IP[50];  // IPC的IP地址
	char s_ipc_UserName[50]; // 用户名
	char s_ipc_Password[50]; // 密码
	LONG g_lRealPort;      // IPC解码库端口，仅大华相机使用
	int m_in_decode_type;  // 设置软硬解码，仅大华相机使用

	//输入信号参数
	int m_in_video_type;//输入视频的编码格式，0：H264, 1:H265
	int m_in_video_fmt;//输入视频的格式，0：YUV420P
	int m_in_video_width;
	int m_in_video_height;
	int m_in_video_framerateD;//输入视频的帧率分母，如25帧每秒，D=1，N=25
	int m_in_video_framerateN;//输入视频的帧率分子

	int m_in_audio_type;//输入音频的编码格式，0:AAC
	int m_in_audio_fmt;//输入音频的格式，0：S16
	int m_in_audio_channels;//输入音频的声道数
	int m_in_audio_sample_rate;//输入音频的采样率

	//相机接收到码流的回调处理类
	CTLEduMRCameraReciver* m_pReciver;
}TLEduMRCameraParam;

struct MRAVFrame;

//相机封装接口类
class DLL_API CTLEduCameraMR
{
public:
	CTLEduCameraMR(void);
	virtual ~CTLEduCameraMR(void);

	BOOL Start();

	void Stop();

	BOOL InitObj(TLEduMRCameraParam* pParam);

	void ReleaseObj();

	// 获取流数据回调函数，老师场景
	static void GetData(LONG lStreamHandle, MRAVFrame *pAvFrame, void *userdata);

	// 获取通道解码数据回调函数，老师场景
	static void GetDecFrame(
		LONG lStreamHandle,
		int stream_index,
		DWORD nTimeTick,
		BYTE *pYBuff,
		BYTE *pUBuff,
		BYTE *pVBuff,
		int nWidth,
		int nHeight,
		int nYStride,
		int nUVStride,
		void *userdata
	);

	// 通道重连回调函数，老师场景
	static int ReLink(LONG lStreamHandle, int   nState, void *userdata);

	BYTE *Recv_Buff;

	static CTLEduCameraMR* CreateObj();
	static void ReleaseObj(CTLEduCameraMR* pObj);

protected:
	LONG pHandle; // 流句柄
	HMODULE hMod_net;      // 明日相机网络连接动态库句柄
	HMODULE hMod_play;      // 明日相机播放动态库句柄
	TLEduMRCameraParam m_in_param;
};
