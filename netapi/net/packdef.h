#pragma once

#include<memory.h>
#include"cjson.h"
#define _DEF_BUFFER         (4096)
#define _DEF_CONTENT_SIZE	(1024)
#define _MAX_SIZE           (40)
#define _MAX_PATH           (260)

#define DEF_HOBBY_COUNT     (8  )
#define MAX_CONTENT_LEN     (4096 )

//自定义协议   先写协议头 再写协议结构
//登录 注册 获取好友信息 添加好友 聊天 发文件 下线请求
#define _DEF_PACK_BASE	(10000)
#define _DEF_PACK_COUNT (100)

#define _DEF_PORT 8080
#define _DEF_SERVERIP "192.168.184.134"
//192.168.184.131  or 127.0.0.1

//注册
#define _DEF_PACK_REGISTER_RQ	(_DEF_PACK_BASE + 0 )
#define _DEF_PACK_REGISTER_RS	(_DEF_PACK_BASE + 1 )
//登录
#define _DEF_PACK_LOGIN_RQ	(_DEF_PACK_BASE + 2 )
#define _DEF_PACK_LOGIN_RS	(_DEF_PACK_BASE + 3 )

//创建房间
#define DEF_PACK_CREATEROOM_RQ  (_DEF_PACK_BASE + 4)
#define DEF_PACK_CREATEROOM_RS  (_DEF_PACK_BASE + 5)
//加入房间
#define DEF_PACK_JOINROOM_RQ  (_DEF_PACK_BASE + 6)
#define DEF_PACK_JOINROOM_RS  (_DEF_PACK_BASE + 7)
//房间列表请求
#define DEF_PACK_ROOM_MEMBER    (_DEF_PACK_BASE + 8)
//音频数据
#define DEF_PACK_AUDIO_FRAME    (_DEF_PACK_BASE + 9)
//视频数据
#define DEF_PACK_VIDEO_FRAME    (_DEF_PACK_BASE + 10)
//退出房间请求
#define DEF_PACK_LEAVEROOM_RQ   (_DEF_PACK_BASE + 11)
//退出房间回复
#define DEF_PACK_LEAVEROOM_RS   (_DEF_PACK_BASE + 12)
//音频注册
#define DEF_PACK_AUDIO_REGISTER (_DEF_PACK_BASE + 13)
//视频注册
#define DEF_PACK_VIDEO_REGISTER (_DEF_PACK_BASE + 14)
//验证码请求
#define DEF_PACK_VEFCODE_RQ   (_DEF_PACK_BASE + 15)
//验证码提交
#define DEF_PACK_VEFCODE_RS   (_DEF_PACK_BASE + 16)
//验证码校验结果
#define DEF_PACK_VEFCODE_RES   (_DEF_PACK_BASE + 17)
#define _DEF_PACK_VIDEO_H264    (_DEF_PACK_BASE + 18 )

//返回的结果
//注册请求的结果
#define tel_is_exist		(0)
#define register_success	(1)
#define name_is_exist       (2)
//登录请求的结果
#define tel_not_exist		(0)
#define password_error		(1)
#define login_success		(2)
#define user_online         (3)
#define vefcode_error       (4)
#define tel_error           (5)
//创建房间结果
#define room_is_exist        0
#define create_success       1

//加入房间结果
#define room_no_exist        0
#define join_success         1




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
        memset(m_name,0,_MAX_SIZE);
	}
	PackType type;
	int result;
	int userid;
    char m_name[_MAX_SIZE];
}STRU_LOGIN_RS;


//创建房间请求
typedef struct STRU_CREATEROOM_RQ
{
    STRU_CREATEROOM_RQ()
    {
        m_nType = DEF_PACK_CREATEROOM_RQ;
        m_UserID = 0;
    }

    PackType m_nType;   //包类型
    int m_UserID;

}STRU_CREATEROOM_RQ;

//创建房间回复
typedef struct STRU_CREATEROOM_RS
{
    STRU_CREATEROOM_RS()
    {
        m_nType= DEF_PACK_CREATEROOM_RS;
        m_lResult = 0;
        m_RoomId = 0;
    }
    PackType m_nType;   //包类型
    int  m_lResult ;    //注册结果
    int  m_RoomId;

}STRU_CREATEROOM_RS;

//加入房间请求
typedef struct STRU_JOINROOM_RQ
{
    STRU_JOINROOM_RQ()
    {
        m_nType = DEF_PACK_JOINROOM_RQ;
        m_UserID = 0;
        m_RoomID = 0;
    }

    PackType m_nType;   //包类型
    int m_UserID;
    int m_RoomID;

}STRU_JOINROOM_RQ;

//加入房间回复
typedef struct STRU_JOINROOM_RS
{
    STRU_JOINROOM_RS()
    {
        m_nType= DEF_PACK_JOINROOM_RS;
        m_lResult = 0;
        m_RoomID = 0;
    }
    PackType m_nType;   //包类型
    int  m_lResult ;    //注册结果
    int m_RoomID;
}STRU_JOINROOM_RS;

//房间成员请求
typedef struct STRU_ROOM_MEMBER_RQ
{
    STRU_ROOM_MEMBER_RQ()
    {
        m_nType= DEF_PACK_ROOM_MEMBER;
        m_UserID =0;
        memset(m_szUser,0,_MAX_SIZE);
    }
    PackType m_nType;   //包类型
    int m_UserID;
    char m_szUser[_MAX_SIZE];

}STRU_ROOM_MEMBER_RQ;


//离开房间请求
typedef struct STRU_LEAVEROOM_RQ
{
    STRU_LEAVEROOM_RQ()
    {
        m_nType = DEF_PACK_LEAVEROOM_RQ;
        m_nUserId = 0;
        m_RoomId = 0;
        memset(szUserName,0,_MAX_SIZE);
    }
    PackType   m_nType;   //包类型
    int    m_nUserId; //用户ID
    int    m_RoomId;
    char   szUserName[_MAX_SIZE];
}STRU_LEAVEROOM_RQ;


typedef struct UserInfo
{
    UserInfo()
       {
            m_sockfd = 0;
            m_id = 0;
            m_roomid = 0;
            m_audiofd=0;
            m_videofd=0;
            memset(m_userName, 0 , _MAX_SIZE);

       }
       int  m_sockfd;
       int  m_id;
       int m_roomid;
       char m_userName[_MAX_SIZE];
       int m_audiofd;
       int m_videofd;
}UserInfo;

struct STRU_AUDIO_REGISTER
{
    STRU_AUDIO_REGISTER():m_nType(DEF_PACK_AUDIO_REGISTER)
    {
        m_userid = 0;
    }
    PackType   m_nType;   //包类型
    int m_userid;

};

struct STRU_VIDEO_REGISTER
{
    STRU_VIDEO_REGISTER():m_nType(DEF_PACK_VIDEO_REGISTER)
    {
        m_userid = 0;
    }
    PackType   m_nType;   //包类型
    int m_userid;
};
//请求验证码
struct STRU_VEFCODE_RQ
{
    STRU_VEFCODE_RQ():m_nType(DEF_PACK_VEFCODE_RQ)
    {
        vefType=1;
        memset(m_tel,0,_MAX_SIZE);
    }
    PackType m_nType;
    int vefType;
    char  m_tel[_MAX_SIZE];
};
//提交验证码
struct STRU_VEFCODE_RS
{
    STRU_VEFCODE_RS():m_nType(DEF_PACK_VEFCODE_RS)
    {
        m_userid=0;
        vefType=2;
        m_res=0;
        memset(m_name,0,_MAX_SIZE);
        memset(vefCode,0,_MAX_SIZE);
        memset(m_tel,0,_MAX_SIZE);
    }
    PackType m_nType;
    int m_userid;
    int vefType;
    int m_res;
    char m_name [_MAX_SIZE];
    char vefCode[_MAX_SIZE];
    char  m_tel[_MAX_SIZE];
};
///音频数据帧
/// 成员描述
/// int type;
/// int userId;
/// int roomId;
/// int min;
/// int sec;
/// int msec;
/// QByteArray audioFrame;
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
///
///

struct STRU_AUDIO_FRAME_V2
{
    STRU_AUDIO_FRAME_V2() {
        type = DEF_PACK_AUDIO_FRAME;
        userid = 0;
        roomid = 0;
        timestamp = 0;
        dataLen = 0;
    }
    int type;
    int userid;
    int roomid;
    int64_t timestamp;
    int dataLen;
    char data[];
};

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




