#include"INet.h"

class Udp : public INet{
public:
	Udp( INetMediator* media);
	~Udp();
	//初始化网络
	 bool InitNet() ;

	//发送数据(udp: ip ulonng 类型，决定发给谁；TCP里socket （uint）决定发给谁 )
	 bool SendData(char* data, int len, unsigned long to) ;
	//udp:sendto(scoket, buf,len,flag,unsigned long to,tolen)
	// tcp:send(scoket, buf,len,flag)

	//接受数据(放在线程里)
	 void recvData() ;


	//关闭网络(回收资源，关闭套接字，卸载库)
	 void CloseInitNet();

	static unsigned __stdcall  RecvThread(void* lpVoid);
private:
	HANDLE m_handle;
	bool m_isRunning;
};