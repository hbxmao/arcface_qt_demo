#pragma once

//�������Դ����ط�װ
//������ݴ���Ļص���
class CTLEduMRCameraReciver
{
public:
	//����յ����ݵĻص�����
	//bVideoΪ1��ʾ��Ƶ����0��ʾ��Ƶ����
	//bEsΪ1��ʾѹ��������0��ʾ��������
	virtual void OnGotFrame(BYTE* pBuf, int buf_len, LONGLONG pts, BOOL bVideo, BOOL bEs) = NULL;
};
