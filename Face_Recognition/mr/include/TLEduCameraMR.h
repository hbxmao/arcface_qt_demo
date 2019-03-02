#pragma once
#include <windows.h>

#ifdef DLL_EXPORTS  
#define DLL_API __declspec(dllexport)  
#else  
#define DLL_API __declspec(dllimport)  
#endif  

class CTLEduMRCameraReciver;

//�����ʼ������
typedef struct TLEduMRCameraParam
{
	//����Ĺ���ģʽ��0��������ģʽ��1������ģʽ��2�����ģʽ��ͬʱ���ѹ������ѹ������
	int m_work_mod;

	//�����������
	char s_ipc_IP[50];  // IPC��IP��ַ
	char s_ipc_UserName[50]; // �û���
	char s_ipc_Password[50]; // ����
	LONG g_lRealPort;      // IPC�����˿ڣ��������ʹ��
	int m_in_decode_type;  // ������Ӳ���룬�������ʹ��

	//�����źŲ���
	int m_in_video_type;//������Ƶ�ı����ʽ��0��H264, 1:H265
	int m_in_video_fmt;//������Ƶ�ĸ�ʽ��0��YUV420P
	int m_in_video_width;
	int m_in_video_height;
	int m_in_video_framerateD;//������Ƶ��֡�ʷ�ĸ����25֡ÿ�룬D=1��N=25
	int m_in_video_framerateN;//������Ƶ��֡�ʷ���

	int m_in_audio_type;//������Ƶ�ı����ʽ��0:AAC
	int m_in_audio_fmt;//������Ƶ�ĸ�ʽ��0��S16
	int m_in_audio_channels;//������Ƶ��������
	int m_in_audio_sample_rate;//������Ƶ�Ĳ�����

	//������յ������Ļص�������
	CTLEduMRCameraReciver* m_pReciver;
}TLEduMRCameraParam;

struct MRAVFrame;

//�����װ�ӿ���
class DLL_API CTLEduCameraMR
{
public:
	CTLEduCameraMR(void);
	virtual ~CTLEduCameraMR(void);

	BOOL Start();

	void Stop();

	BOOL InitObj(TLEduMRCameraParam* pParam);

	void ReleaseObj();

	// ��ȡ�����ݻص���������ʦ����
	static void GetData(LONG lStreamHandle, MRAVFrame *pAvFrame, void *userdata);

	// ��ȡͨ���������ݻص���������ʦ����
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

	// ͨ�������ص���������ʦ����
	static int ReLink(LONG lStreamHandle, int   nState, void *userdata);

	BYTE *Recv_Buff;

	static CTLEduCameraMR* CreateObj();
	static void ReleaseObj(CTLEduCameraMR* pObj);

protected:
	LONG pHandle; // �����
	HMODULE hMod_net;      // ��������������Ӷ�̬����
	HMODULE hMod_play;      // ����������Ŷ�̬����
	TLEduMRCameraParam m_in_param;
};
