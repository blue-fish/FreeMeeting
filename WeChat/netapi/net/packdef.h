#pragma once

#include<memory.h>
#include<stdint.h>

#define _DEF_BUFFER         (4096)
#define _DEF_CONTENT_SIZE	(1024)
#define _MAX_SIZE           (40)
#define _MAX_PATH           (260)

//自定义协议   先写协议头 再写协议结构
//登录 注册 获取好友信息 添加好友 聊天 发文件 下线请求

#define _DEF_PORT 8000
#define _DEF_SERVERIP  "192.168.184.133"  //"192.168.42.131"

#define _DEF_PACK_BASE	(10000)
#define _DEF_PACK_COUNT (100)

//注册
#define _DEF_PACK_REGISTER_RQ	(_DEF_PACK_BASE + 0 )
#define _DEF_PACK_REGISTER_RS	(_DEF_PACK_BASE + 1 )
//登录
#define _DEF_PACK_LOGIN_RQ      (_DEF_PACK_BASE + 2 )
#define _DEF_PACK_LOGIN_RS      (_DEF_PACK_BASE + 3 )
//创建房间
#define _DEF_PACK_CREATEROOM_RQ (_DEF_PACK_BASE + 4 )
#define _DEF_PACK_CREATEROOM_RS (_DEF_PACK_BASE + 5 )
//加入房间
#define _DEF_PACK_JOINROOM_RQ   (_DEF_PACK_BASE + 6 )
#define _DEF_PACK_JOINROOM_RS   (_DEF_PACK_BASE + 7 )
//房间成员请求
#define _DEF_PACK_ROOM_MEMBER   (_DEF_PACK_BASE + 8 )
//离开房间请求
#define _DEF_PACK_LEAVEROOM_RQ  (_DEF_PACK_BASE + 9 )
//音频数据
#define _DEF_PACK_AUDIO_FRAME   (_DEF_PACK_BASE + 10 )
//视频数据
#define _DEF_PACK_VIDEO_FRAME   (_DEF_PACK_BASE + 11 )
//音频注册
#define _DEF_PACK_AUDIO_REGISTER   (_DEF_PACK_BASE + 12 )
//视频注册
#define _DEF_PACK_VIDEO_REGISTER   (_DEF_PACK_BASE + 13 )

//返回的结果
//注册请求的结果
#define tel_is_exist		(0)
#define register_success	(1)
#define name_is_exist       (2)
//登录请求的结果
#define user_not_exist		(0)
#define password_error		(1)
#define login_success		(2)


typedef int PackType;

//协议结构
//注册
typedef struct STRU_REGISTER_RQ
{
	STRU_REGISTER_RQ():type(_DEF_PACK_REGISTER_RQ)
	{
		memset( tel  , 0, sizeof(tel));
		memset( name  , 0, sizeof(name));
		memset( password , 0, sizeof(password) );
	}
	//需要手机号码 , 密码, 昵称
	PackType type;
	char tel[_MAX_SIZE];
	char name[_MAX_SIZE];
	char password[_MAX_SIZE];

}STRU_REGISTER_RQ;

typedef struct STRU_REGISTER_RS
{
	//回复结果
	STRU_REGISTER_RS(): type(_DEF_PACK_REGISTER_RS) , result(register_success)
	{
	}
	PackType type;
	int result;

}STRU_REGISTER_RS;

//登录
typedef struct STRU_LOGIN_RQ
{
	//登录需要: 手机号 密码 
	STRU_LOGIN_RQ():type(_DEF_PACK_LOGIN_RQ)
	{
		memset( tel , 0, sizeof(tel) );
		memset( password , 0, sizeof(password) );
	}
	PackType type;
	char tel[_MAX_SIZE];
	char password[_MAX_SIZE];

}STRU_LOGIN_RQ;

typedef struct STRU_LOGIN_RS
{
	// 需要 结果 , 用户的id
	STRU_LOGIN_RS(): type(_DEF_PACK_LOGIN_RS) , result(login_success),userid(0)
	{
        memset( name , 0, _MAX_SIZE );
	}
	PackType type;
	int result;
	int userid;
    char name[_MAX_SIZE];
}STRU_LOGIN_RS;

//创建房间请求
typedef struct STRU_CREATEROOM_RQ
{
    STRU_CREATEROOM_RQ(){
        type=_DEF_PACK_CREATEROOM_RQ;
        userid=0;
    }
    PackType type;
    int userid;
}STRU_CREATEROOM_RQ;

//创建房间回复
typedef struct STRU_CREATEROOM_RS
{
    STRU_CREATEROOM_RS(){
        type=_DEF_PACK_CREATEROOM_RS;
        result=0;
        roomid=0;
    }
    PackType type;
    int result;
    int roomid;
}STRU_CREATEROOM_RS;

//加入房间请求
typedef struct STRU_JOINROOM_RQ
{
    STRU_JOINROOM_RQ(){
        type=_DEF_PACK_JOINROOM_RQ;
        userid=0;
        roomid=0;
    }
    PackType type;
    int userid;
    int roomid;
}STRU_JOINROOM_RQ;

//加入房间回复
typedef struct STRU_JOINROOM_RS
{
    STRU_JOINROOM_RS(){
        type=_DEF_PACK_JOINROOM_RS;
        result=0;
        roomid=0;
    }
    PackType type;
    int result;
    int roomid;
}STRU_JOINROOM_RS;

//房间成员请求
typedef struct STRU_ROOM_MEMBER_RQ
{
    // 需要 结果 , 用户的id
    STRU_ROOM_MEMBER_RQ(): type(_DEF_PACK_ROOM_MEMBER) , userid(0)
    {
        memset( name , 0, _MAX_SIZE );
    }
    PackType type;
    int userid;
    char name[_MAX_SIZE];
}STRU_ROOM_MEMBER_RQ;

//离开房间
typedef struct STRU_LEAVEROOM_RQ
{
    // 需要 结果 , 用户的id
    STRU_LEAVEROOM_RQ(): type(_DEF_PACK_LEAVEROOM_RQ) ,roomid(0), userid(0)
    {
        memset( name , 0, _MAX_SIZE );
    }
    PackType type;
    int userid;
    int roomid;
    char name[_MAX_SIZE];
}STRU_LEAVEROOM_RQ;

//音频注册
typedef struct STRU_AUDIO_REGISTER
{
    STRU_AUDIO_REGISTER():type(_DEF_PACK_AUDIO_REGISTER){
        userid=0;
    }
    PackType type;
    int userid;
}STRU_AUDIO_REGISTER;

//视频注册
typedef struct STRU_VIDEO_REGISTER
{
    STRU_VIDEO_REGISTER():type(_DEF_PACK_VIDEO_REGISTER){
        userid=0;
    }
    PackType type;
    int userid;
}STRU_VIDEO_REGISTER;

///音频数据帧
///成员描述
/// int type;
/// int userId;
/// int roomId;
/// int min;
/// int sec;
/// int msec;
/// QByteArray audioFrame; -->char frame[];柔性数组
///


///视频数据帧
/// 成员描述
/// int type;
/// int userId;
/// int roomId;
/// int min;
/// int sec;
/// int msec;
/// QByteArray videoFrame;



#define _DEF_PACK_VIDEO_H264    (_DEF_PACK_BASE + 14 )
//// 添加H.264视频数据包结构
//struct STRU_VIDEO_H264
//{
//    STRU_VIDEO_H264() {
//        type = _DEF_PACK_VIDEO_H264;
//        userid = 0;
//        roomid = 0;
//        width = 0;
//        height = 0;
//        pts = 0;
//        frameType = 0; // 0:P帧 1:I帧
//        dataLen = 0;
//    }
//    int type;
//    int userid;
//    int roomid;
//    int width;
//    int height;
//    int64_t pts;
//    int frameType;
//    int dataLen;
//    char data[]; // 柔性数组存储H.264数据
//};

// 音频数据帧 - 使用64位时间戳
typedef struct STRU_AUDIO_FRAME
{
    STRU_AUDIO_FRAME() {
        type = _DEF_PACK_AUDIO_FRAME;
        userid = 0;
        roomid = 0;
        timestamp = 0;
        dataLen = 0;
    }
    int type;
    int userid;
    int roomid;
    int64_t timestamp;  // 64位时间戳，毫秒级
    int dataLen;
    char data[];  // 柔性数组
} STRU_AUDIO_FRAME;
// 修改H264视频结构，确保时间戳一致
struct STRU_VIDEO_H264_V2
{
    STRU_VIDEO_H264_V2() {
        type = _DEF_PACK_VIDEO_H264;
        userid = 0;
        roomid = 0;
        width = 0;
        height = 0;
        timestamp = 0;  // 使用统一的时间戳
        frameType = 0;
        dataLen = 0;
    }
    int type;
    int userid;
    int roomid;
    int width;
    int height;
    int64_t timestamp;  // 改为timestamp而不是pts
    int frameType;
    int dataLen;
    char data[];
};
