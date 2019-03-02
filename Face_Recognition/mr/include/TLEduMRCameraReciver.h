#pragma once

//相机输入源的相关封装
//相机数据处理的回调类
class CTLEduMRCameraReciver
{
public:
	//相机收到数据的回调函数
	//bVideo为1表示视频流，0表示音频流；
	//bEs为1表示压缩码流，0表示解码码流
	virtual void OnGotFrame(BYTE* pBuf, int buf_len, LONGLONG pts, BOOL bVideo, BOOL bEs) = NULL;
};
