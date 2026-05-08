#pragma once
#include<string.h>
//加载库的版本号

#define DEF_VERSION_LOW  (2)
#define DEF_VERSION_HIGH (2)

#define DEF_UDP_PORT  (12345)
#define DEF_TCP_PORT  (8080)

#define _DEF_SERVERIP "192.168.184.131"

//tcp协议监听队列的最大长度
#define _DEF_TCP_LISTEN_MAX (10)

//昵称 手机号，密码的最大长度
#define _DEF_MAX_LENTH (15)

//聊天内容长度*
#define _DEF_CONTENT_LENTH  (8*1024)


//声明结果宏
//注册结果
#define _def_register_success     (0)
#define _def_register_tel_exists  (1)
#define _def_register_name_exists (2)

//登录结果
#define _def_login_success            (0)
#define _def_login_tel_not_exists     (1)
#define _def_login_password_error     (2)

//添加好友的结果
#define _def_add_friend_success        (0)
#define _def_add_friend_offline        (1)
#define _def_add_friend_refuse         (2)
#define _def_add_friend_not_exists     (3)

//发送结果
#define _def_send_success              (0)
#define _def_send_fail                 (1)

#define _DEF_PROTOCOL_COUNT				(100)

#define _DEF_PROTOCOL_BASE				    (1000)
//注册请求
#define _DEF_REGISTER_RQ			    (_DEF_PROTOCOL_BASE+1)
//注册回复
#define _DEF_REGISTER_RS				(_DEF_PROTOCOL_BASE+2)
//登录请求
#define _DEF_LOGIN_RQ				    (_DEF_PROTOCOL_BASE+3)
//登录回复
#define _DEF_LOGIN_RS				    (_DEF_PROTOCOL_BASE+4)
//添加好友请求
#define _DEF_ADD_FRIEND_RQ				(_DEF_PROTOCOL_BASE+5)
//添加好友回复
#define _DEF_ADD_FRIEND_RS				(_DEF_PROTOCOL_BASE+6)
//聊天请求
#define _DEF_CHAT_RQ				    (_DEF_PROTOCOL_BASE+7)
//聊天回复
#define _DEF_CHAT_RS				    (_DEF_PROTOCOL_BASE+8)
//下线请求
#define _DEF_OFFLINE_RQ				    (_DEF_PROTOCOL_BASE+9)
//用户信息
#define _DEF_FRIEND_INFO				(_DEF_PROTOCOL_BASE+10)



//在线状态
#define _def_status_online              (0)
#define _def_status_offline             (1)


// 声明结构体类型
typedef int packtype;
//请求结构体
//注册请求
typedef struct _STRU_REGISTER_RQ {
    _STRU_REGISTER_RQ():type(_DEF_REGISTER_RQ)
    {
        memset(tel, 0, _DEF_MAX_LENTH);
        memset(password, 0, _DEF_MAX_LENTH);
        memset(name, 0, _DEF_MAX_LENTH);
    }
    packtype type;
    char tel[_DEF_MAX_LENTH];
    char password[_DEF_MAX_LENTH];
    char name[_DEF_MAX_LENTH];
}_STRU_REGISTER_RQ;
//注册回复(成功和失败两种情况)
typedef struct _STRU_REGISTER_RS {
    _STRU_REGISTER_RS():result(_def_register_name_exists), type(_DEF_REGISTER_RS)
    {
    }
    packtype type;
    int result;
}_STRU_REGISTER_RS;

//登录请求
typedef struct _STRU_LOGIN_RQ {
    _STRU_LOGIN_RQ():type(_DEF_LOGIN_RQ)
    {
        memset(tel, 0, _DEF_MAX_LENTH);
        memset(password, 0, _DEF_MAX_LENTH);

    }
    packtype type;
    char tel[_DEF_MAX_LENTH];
    char password[_DEF_MAX_LENTH];

}_STRU_LOGIN_RQ;
//登录回复
typedef struct _STRU_LOGIN_RS {
    _STRU_LOGIN_RS() :type(_DEF_LOGIN_RS),userId(0),result(_def_login_tel_not_exists)
    {
    }
    packtype type;
    int userId;
    int result;
}_STRU_LOGIN_RS;
//添加好友(好友昵称，自己的id,自己的昵称)
typedef struct _STRU_ADD_FRIEND_RQ
{
    _STRU_ADD_FRIEND_RQ():userId(0), type(_DEF_ADD_FRIEND_RQ)
    {
        memset(userName, 0, _DEF_MAX_LENTH);
        memset(friendName, 0, _DEF_MAX_LENTH);
    }
    packtype type;
    int userId;
    char userName[_DEF_MAX_LENTH];
    char friendName[_DEF_MAX_LENTH];
}_STRU_ADD_FRIEND_RQ;
//添加好友回复(添加成功，好友不存在，好友拒绝，好友不在线)
typedef struct _STRU_ADD_FRIEND_RS
{
    _STRU_ADD_FRIEND_RS() :userId(0), friendId(0),result(_def_add_friend_not_exists), type(_DEF_ADD_FRIEND_RS)
    {
        memset(userName, 0, _DEF_MAX_LENTH);
        memset(friendName, 0, _DEF_MAX_LENTH);
    }
    packtype type;
    int userId;
    int result;
    int friendId;
    char userName[_DEF_MAX_LENTH];
    char friendName[_DEF_MAX_LENTH];
}_STRU_ADD_FRIEND_RS;
//聊天请求: 聊天内容，自己的id，好友的id
typedef struct _STRU_CHAT_RQ {
    _STRU_CHAT_RQ():userId(0),friendId(0), type(_DEF_CHAT_RQ)
    {
        memset(content, 0, _DEF_CONTENT_LENTH);
    }
    packtype type;
    char content[_DEF_CONTENT_LENTH];
    int userId;
    int friendId;
}_STRU_CHAT_RQ;


//聊天回复
typedef struct _STRU_CHAT_RS {
    _STRU_CHAT_RS() :result(_def_send_fail), type(_DEF_CHAT_RS),friendId(0)
    {

    }
    packtype type;
    int result;
    int friendId;
}_STRU_CHAT_RS;

//下线请求
typedef struct _STRU_OFFLINE_RQ {
    _STRU_OFFLINE_RQ()  :userId(0),type(_DEF_OFFLINE_RQ)
    {

    }
    packtype type;
    int userId;

}_STRU_OFFLINE_RQ;

//用户信息：type,ID,昵称，签名，头像id，状态
typedef struct _STRU_FRIEND_INFO {
    _STRU_FRIEND_INFO() :type(_DEF_OFFLINE_RQ), id(0),iconId(0),status(_def_status_offline)
    {
        memset(name, 0, _DEF_MAX_LENTH);
        memset(feeling, 0, _DEF_MAX_LENTH);
    }
    packtype type;
    int id;
    int iconId;
    int status;
    char name[_DEF_MAX_LENTH];
    char feeling[_DEF_MAX_LENTH];

}_STRU_FRIEND_INFO;
