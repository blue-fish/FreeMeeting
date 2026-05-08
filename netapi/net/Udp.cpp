# include "Udp.h"
#include"def.h"
#include"../Mediator/UdpMediator.h"
unsigned __stdcall Udp::RecvThread(void* lpVoid)
{
	Udp* pThis = (Udp*)lpVoid;
	pThis->recvData();
	return 1;
}
Udp::Udp(INetMediator* media):m_handle(nullptr),m_isRunning(true)
{
	m_Mediator = media;
}
Udp::~Udp()
{

}
//初始化网络
bool Udp::InitNet()
{
	cout << __func__ << endl;
	//加载库
		//数字定义成宏
	WORD version = MAKEWORD(DEF_VERSION_HIGH, DEF_VERSION_LOW);
	WSADATA data = {};
	int err = WSAStartup(version,&data);
	if (err!=0)
	{
		cout << "WSAStartUp failed " << endl;
		return false;
	}
		//判断版本号是否正确
	if (DEF_VERSION_HIGH == HIBYTE(data.wVersion) && DEF_VERSION_LOW == LOBYTE(data.wVersion))
	{
		cout << "WSAStartUp success" << endl;
	}
	else
	{
		cout << "WSAStartUp success" << endl;
		return false;
	}
	//创建套接字
	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == m_socket)
	{
		cout << "socket error:" << WSAGetLastError() << endl;
		return false;
	}
	else
	{
		cout << "socket success" << endl;
	}
	//绑定ip端口
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(DEF_UDP_PORT);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	err = bind(m_socket, (sockaddr*)&addr, sizeof(addr));
	if (SOCKET_ERROR == err)
	{
		cout << "bind error:" << WSAGetLastError() << endl;
		return false;
	}
	else
	{
		cout << "bind success" << endl;
	}
	//创建接受数据的线程
	//CreateThread和ExiteThread是一对线程函数，ExitThread 在退出线程的时候不会回收申请的空间，一些C++运行时函数会申请空间但不会自动释放（如strcpy）
	//而ExiteThread也不会自动释放，会造成内存泄漏
	//_beginthreadex和_endthreadex _endthreadex会回收空间后调用_endthreadex；
	m_handle=(HANDLE)_beginthreadex(
		0,/*线程安全级别，0是默认使用的安全级别*/
		0,/*默认堆栈大小1M*/
		&RecvThread,/*线程要执行的函数的起始地址*/
		this,/*执行函数的参数列表*/
		0,/*是否立即执行*/
		nullptr/*操作系统分配的线程id*/);
	return true;
}


//发送数据(udp: ip ulonng 类型，决定发给谁；TCP里socket （uint）决定发给谁 )
bool Udp::SendData(char* data, int len, unsigned long to)
{
	cout << __func__ << endl;
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(DEF_UDP_PORT);
	addr.sin_addr.S_un.S_addr = to;
	int nSendNum = sendto(m_socket, data, len, 0, (sockaddr*)&addr, sizeof(addr));
	if (SOCKET_ERROR == nSendNum)
	{
		cout << "sendto error :" << WSAGetLastError() << endl;
		return false;
	}
	return true;
}
//udp:sendto(scoket, buf,len,flag,unsigned long to,tolen)
// tcp:send(scoket, buf,len,flag)

//接受数据(放在线程里,一直循环等待接受数据)
void Udp::recvData()
{
	cout << __func__ << endl;
	int nRecvNum = 0;
	char recvBuf[4096]="";
	sockaddr_in addrFrom = {};
	int size = sizeof(addrFrom);
	while (m_isRunning)
	{
		cout << __func__ << endl;
		nRecvNum = recvfrom(m_socket,recvBuf,sizeof(recvBuf),0,(sockaddr*)&addrFrom,&size);
		if (nRecvNum > 0)
		{
			//接受一个数据包成功
			//根据接受数据的大小申请一个新的空间
			char* pPack = new char[nRecvNum];
			//把接受到的数据拷贝到新的空间
			memcpy(pPack,recvBuf,nRecvNum);//寄存器拷贝 按照长度拷贝，不考虑数据类型，strcpy()只能拷贝字符
			//TODO : 把接受到的数据传给中介类
			m_Mediator->transmitData(pPack, nRecvNum, addrFrom.sin_addr.S_un.S_addr);
		}
		else
		{
			cout << "recvFrom error:" << WSAGetLastError() << endl;
			break;
		}
	}

}
//关闭网络(回收资源，关闭套接字，卸载库)
void Udp::CloseInitNet()
{
	cout << __func__ << endl;
	//回收线程资源
	// 创建线程时，操作系统分配给线程的资源：句柄、线程id、内核对象，引用计数器 2；
	//想要回收资源，需要计数器变为0；结束线程工作：关闭句柄
	m_isRunning =  false;
	if (m_handle)
	{
		if (WAIT_TIMEOUT == WaitForSingleObject(m_handle, 1000)) //等待的线程还在继续运行
		{
			//杀死进程
			TerminateThread(m_handle, -1);
		}
		CloseHandle(m_handle);
		m_handle = nullptr;
	}
	//关闭套接字
	if (!m_socket && INVALID_SOCKET != m_socket)
	{
		closesocket(m_socket);
	}
	//卸载库
	WSACleanup();
}