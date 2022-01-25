// ALPC_Server.cpp : Defines the entry point for the console application.
//


#include "stdafx.h"
#include "windows.h"
#include "stdio.h"

#pragma comment(lib,"ntdll.lib")

#define ulong unsigned long
#define ulonglong unsigned long long
#define ULONG unsigned long
#define ULONGLONG unsigned long long
#define ushort unsigned short
#define USHORT unsigned short

typedef enum _LPC_TYPE{
	LPC_NEW_MESSAGE, // A new message
	LPC_REQUEST, // A request message
	LPC_REPLY, // A reply to a request message
	LPC_DATAGRAM, //
	LPC_LOST_REPLY, //
	LPC_PORT_CLOSED, // Sent when port is deleted
	LPC_CLIENT_DIED, // Messages to thread termination ports
	LPC_EXCEPTION, // Messages to thread exception port
	LPC_DEBUG_EVENT, // Messages to thread debug port
	LPC_ERROR_EVENT, // Used by ZwRaiseHardError
	LPC_CONNECTION_REQUEST // Used by ZwConnectPort
}LPC_TYPE;



struct _UNICODE_STRING
{
	unsigned short Length;
	unsigned short MaxLength;
	unsigned long Pad;
	wchar_t* Buffer;
};
typedef struct _OBJECT_ATTRIBUTES {
  ULONGLONG           Length;
  HANDLE          RootDirectory;
  _UNICODE_STRING* ObjectName;
  ULONGLONG           Attributes;
  PVOID           SecurityDescriptor;
  PVOID           SecurityQualityOfService;
} OBJECT_ATTRIBUTES;




struct _ALPC_PORT_ATTRIBUTES
{
   ulong Flags;
   _SECURITY_QUALITY_OF_SERVICE SecurityQos;
   ulonglong MaxMessageLength;
   ulonglong MemoryBandwidth;
   ulonglong MaxPoolUsage;
   ulonglong MaxSectionSize;
   ulonglong MaxViewSize;
   ulonglong MaxTotalSectionSize;
   ulong DupObjectTypes;
   ulong Reserved;
};

struct _CLIENT_ID
{
	unsigned long long UniqueProcess;
	unsigned long long UniqueThread;
};
typedef struct _PORT_MESSAGE
{
	union //at 0x0
	{
		unsigned long Length;
		struct
		{
			unsigned short DataLength;
			unsigned short TotalLength;
		}s1;
	}u1;
	union //at 0x4
	{
		unsigned long ZeroInit;
		struct
		{
			unsigned short Type;
			unsigned short DataInfoOffset;
		}s2;
	}u2;
	_CLIENT_ID ClientId;//at 0x8
	unsigned long MessageId;//at 0x18
	unsigned long Pad;//at 0x1C
	union //at 0x20
	{
		unsigned long long ClientViewSize;
		unsigned long CallbackId;
	}u3;
}PORT_MESSAGE;

struct _REMOTE_PORT_VIEW
{
	unsigned long Length;
	unsigned long Pad;
	unsigned long long ViewSize;
	unsigned long long ViewBase;
};

typedef struct _PORT_VIEW
{
     ULONG Length;
	 ULONG Pad1;
     HANDLE SectionHandle;
     ULONG SectionOffset;
	 ULONG Pad2;
     ULONGLONG ViewSize;
     ULONGLONG ViewBase;
     ULONGLONG ViewRemoteBase;
}PORT_VIEW;


struct _ALPC_MESSAGE_ATTRIBUTES
{ 
	ulong AllocatedAttributes;
	ulong ValidAttributes;
};


//Taken from conhost.exe
struct _CONTEXT_X
{
	_LIST_ENTRY List0;//AT 0x0
	HANDLE hRemoteProcess;//at 0x10
	ulong Unk;//At 0x18
	ulong unkx;//at 0x1C
	_CLIENT_ID ClientId;//at 0x20
	ulonglong Pad0;//at 0x30
	ulonglong Pad1;//at 0x38
	_LIST_ENTRY List1;//at 0x40
};

extern "C"
{

	int ZwClose(HANDLE Handle);

	int	AlpcInitializeMessageAttribute (ulonglong AttributeFlags,  
		_ALPC_MESSAGE_ATTRIBUTES* Buffer, 
		ulonglong BufferSize, 
		ulonglong* RequiredBufferSize);

	ulonglong	AlpcGetMessageAttribute (_ALPC_MESSAGE_ATTRIBUTES* Buffer, ulonglong AttributeFlag);


	int ZwCreatePort (HANDLE* pPortHandle,
					_OBJECT_ATTRIBUTES* ObjectAttributes,
					ulonglong MaxConnectInfoLength,
					ulonglong MaxDataLength,

					void* Reserved);
	int ZwAlpcCreatePort(HANDLE* pPortHandle, _OBJECT_ATTRIBUTES* ObjectAttributes, _ALPC_PORT_ATTRIBUTES* PortAttributes);

	int ZwReplyWaitReceivePort(HANDLE PortHandle,
								void** PortContext,
								_PORT_MESSAGE* ReplyMessage, 
								_PORT_MESSAGE* ReceiveMessage);

	int ZwAcceptConnectPort
							(HANDLE* PortHandle,
							void* PortContext,
							_PORT_MESSAGE* ConnectionRequest,
							bool AcceptConnection,
							void* ServerView,
							_REMOTE_PORT_VIEW* ClientView);


	int ZwReplyPort(HANDLE PortHandle, _PORT_MESSAGE* ReplyMessage);

	int ZwRequestWaitReplyPort
							(HANDLE PortHandle,
							_PORT_MESSAGE* RequestMessage,
							_PORT_MESSAGE* ReplyMessage);
	int ZwOpenProcess
		(HANDLE* ProcessHandle,
		ACCESS_MASK DesiredAccess,
		_OBJECT_ATTRIBUTES* ObjectAttributes,
		_CLIENT_ID* ClientId);

	int ZwAlpcOpenSenderProcess( HANDLE* ProcessHandle,
		HANDLE PortHandle,
		_PORT_MESSAGE* PortMessage,
		ulonglong Flags,
		ACCESS_MASK DesiredAccess,
		_OBJECT_ATTRIBUTES* ObjectAttributes );
	
	int ZwAlpcOpenSenderThread( HANDLE* ThreadHandle,
		HANDLE PortHandle,
		_PORT_MESSAGE* PortMessage,
		ulonglong Flags,
		ACCESS_MASK DesiredAccess,
		_OBJECT_ATTRIBUTES* ObjectAttributes );

	int ZwListenPort(HANDLE PortHandle, _PORT_MESSAGE* ConnectionRequest);
	int ZwReplyPort(HANDLE PortHandle,  _PORT_MESSAGE* ReplyMessage);

	int ZwAlpcSendWaitReceivePort(HANDLE PortHandle, ulonglong Flags,
									 _PORT_MESSAGE* SendMessage,  _ALPC_MESSAGE_ATTRIBUTES* SendMessageAttributes,
									 _PORT_MESSAGE* ReceiveMessage, ulonglong* BufferLength,
									 _ALPC_MESSAGE_ATTRIBUTES* ReceiveMessageAttributes,_LARGE_INTEGER* Timeout);

	int ZwAlpcAcceptConnectPort(HANDLE* PortHandle,
					HANDLE ConnectionPortHandle,
					ulonglong Flags,
					_OBJECT_ATTRIBUTES* ObjectAttributes,
					_ALPC_PORT_ATTRIBUTES* PortAttributes,
					void* PortContext,
					_PORT_MESSAGE* ConnectionRequest,
					_ALPC_MESSAGE_ATTRIBUTES* ConnectionMessageAttributes,
					bool AcceptConnection);
}


#define MaxMsgLen 0x458

int _tmain(int argc, _TCHAR* argv[])
{
	HANDLE hPort = 0;

	wchar_t* PortName = L"\\BaseNamedObjects\\portw";



	_UNICODE_STRING UniS = {0};
	UniS.Length = wcslen(PortName)*2;
	UniS.MaxLength = (UniS.Length)+2;
	UniS.Buffer = PortName;

	_OBJECT_ATTRIBUTES ObjAttr = {sizeof(ObjAttr)};
	ObjAttr.Attributes=0x40;//insensitive
	ObjAttr.ObjectName=&UniS;
	//ObjAttr.RootDirectory=0;

	_ALPC_PORT_ATTRIBUTES PortAtt = {0};
	PortAtt.Flags = 0x20000;
	PortAtt.MaxMessageLength = MaxMsgLen;
	PortAtt.MemoryBandwidth = 0;
	PortAtt.MaxPoolUsage = 0x11600;

	int retValue = ZwAlpcCreatePort(&hPort,&ObjAttr,&PortAtt);
	if(retValue < 0)
	{
		printf ("Error creating ALPC port, ret: %X\r\n",retValue);
		return -5;
	}
	printf("hPort: %X\r\n",hPort);
	//--------------------
	bool bFirst = true;
	_PORT_MESSAGE* pReceiveMessage = 0;
	while(1)
	{
		_ALPC_MESSAGE_ATTRIBUTES* pReceiveMsgAttributes = 
			(_ALPC_MESSAGE_ATTRIBUTES*)LocalAlloc(LMEM_ZEROINIT,0x78);
		ulonglong ReqLength = 0;
		
		int retYY = AlpcInitializeMessageAttribute(0x30000001,pReceiveMsgAttributes,0x78,&ReqLength);
		if(retYY < 0)
		{
			printf("AlpcInitializeMessageAttribute, ret: %X\r\n",retYY);
			ExitProcess(0);
		}

		ulonglong BufferLength = 0x458;
		
		if(bFirst)
		{
			
			pReceiveMessage = (_PORT_MESSAGE*)VirtualAlloc(0,BufferLength,MEM_COMMIT,PAGE_READWRITE);
			bFirst = false;
		}

	


		retValue = ZwAlpcSendWaitReceivePort(hPort, 0 /*Flags*/,
			0,0,
			pReceiveMessage,&BufferLength,
			pReceiveMsgAttributes,0 /* TimeOut */);

		printf("ZwAlpcSendWaitReceivePort, ret: %X\r\n",retValue);
		

		ulonglong AttX = AlpcGetMessageAttribute(pReceiveMsgAttributes,0x20000000);


		if(retValue >= 0)
		{
			if(retValue == 0x102)//STATUS_TIMEOUT
			{
				printf("Timeout\r\n");
				ExitProcess(0);
			}
			else
			{
				AttX = AlpcGetMessageAttribute(pReceiveMsgAttributes,0x10000000);

				HANDLE hShit =(HANDLE) ( *(ulonglong*)(AttX+0x8) );

				ulong Typ = (pReceiveMessage->u2.s2.Type) & 0xFF;
				printf("Type: %X\r\n",Typ);
				if(Typ == 0xA)//LPC_CONNECTION_REQUEST
				{
					printf("New Connection\r\n");
					HANDLE hPort_Remote = 0;


					retValue = ZwAlpcAcceptConnectPort(&hPort_Remote,hPort,0,0,0,0,pReceiveMessage,0,true);
					printf("ZwAlpcAcceptConnectPort, ret: %X, hPort_Remote: %I64X\r\n",retValue,hPort_Remote);

				}
				else if(Typ == 1)//LPC_REQUEST
				{
					printf("New Request\r\n");
					_OBJECT_ATTRIBUTES ObjAtt_SenderP={sizeof(_OBJECT_ATTRIBUTES)};
					_OBJECT_ATTRIBUTES ObjAtt_SenderT={sizeof(_OBJECT_ATTRIBUTES)};

					//HANDLE hProcess = 0, hThread = 0;
					//retValue = ZwAlpcOpenSenderProcess(&hProcess,hPort,pReceiveMessage,0 /* Flags */,0x400 /*Desired */,&ObjAtt_SenderP);
					//printf("ZwAlpcOpenSenderProcess, ret: %X, hProcess: %I64X\r\n",retValue,hProcess);


					//retValue = ZwAlpcOpenSenderThread(&hThread,hPort,pReceiveMessage,0 /* Flags */,0,&ObjAtt_SenderT);
					//printf("ZwAlpcOpenSenderThread, ret: %X, hThread: %I64X\r\n",retValue,hThread);

					ulong Offset = pReceiveMessage->u2.s2.DataInfoOffset;
					char* pData = ((char*)pReceiveMessage) + sizeof(_PORT_MESSAGE) + Offset;
					printf("Data: %s\r\n",pData);

					ulonglong BufferLengthX = 0x458;
					_PORT_MESSAGE* pSendMessage = (_PORT_MESSAGE*)VirtualAlloc(0,BufferLengthX,MEM_COMMIT,PAGE_READWRITE);
					memcpy(pSendMessage,pReceiveMessage,BufferLengthX);


					retValue = ZwAlpcSendWaitReceivePort(hPort,0x10000 /*Flags*/,pSendMessage,0,0,0,0,0);

				}
				else if(Typ == LPC_DATAGRAM)//from ZwRequestPort
				{

				}
			}
		}
		else
		{
			printf("Closing port, bye\r\n");
			ZwClose(hPort);
			ExitProcess(0);
		}



		
	}
	VirtualFree(pReceiveMessage,0,MEM_RELEASE);
	return 0;
}




int _tmainX(int argc, _TCHAR* argv[])
{
	printf("%X\r\n",sizeof(_PORT_MESSAGE));
	HANDLE hPort = 0;



	wchar_t* PortName = L"\\BaseNamedObjects\\portw";



	_UNICODE_STRING UniS = {0};
	UniS.Length = wcslen(PortName)*2;
	UniS.MaxLength = (UniS.Length)+2;
	UniS.Buffer = PortName;

	_OBJECT_ATTRIBUTES ObjAttr = {sizeof(ObjAttr)};
	ObjAttr.Attributes=0x40;//insensitive
	ObjAttr.ObjectName=&UniS;
	//ObjAttr.RootDirectory=0;

	ulonglong MaxConnectInfoLength = 0;
	ulonglong MaxDataLength = 0xC0;
	int retValue = ZwCreatePort(&hPort,&ObjAttr,MaxConnectInfoLength,MaxDataLength,0 /*Reserved*/);
	if(retValue < 0)
	{
		printf ("Error creating ALPC port, ret: %X\r\n",retValue);
		return -5;
	}

	printf("hPort: %X\r\n",hPort);


	void* PortContext = 0;
	_PORT_MESSAGE ReceiveMsg = {0};

	//blocking call
	retValue = ZwReplyWaitReceivePort
								(hPort,
								&PortContext,
								0,
								&ReceiveMsg	);

	printf("ZwReplyWaitReceivePort, ret: %X\r\n",retValue);
	//Check	ReceiveMsg.u2.s2.Type
	unsigned short Typ = ReceiveMsg.u2.s2.Type;
	unsigned long TotalLength = ReceiveMsg.u1.s1.TotalLength;
	unsigned long DataLength = ReceiveMsg.u1.s1.DataLength;
	unsigned long DataInfoOffset = ReceiveMsg.u2.s2.DataInfoOffset;

	

	ulonglong PID = ReceiveMsg.ClientId.UniqueProcess;
	ulonglong TID = ReceiveMsg.ClientId.UniqueThread;

	printf("Type: %X\r\n",Typ);
	printf("TotalLength: %X\r\n",TotalLength);
	printf("DataLength: %X\r\n",DataLength);
	printf("DataInfoOffset: %X\r\n",DataInfoOffset);
	printf("PID: %I64X\r\n",ReceiveMsg.ClientId.UniqueProcess);
	printf("TID: %I64X\r\n",ReceiveMsg.ClientId.UniqueThread);
	printf("MsgId: %X\r\n",ReceiveMsg.MessageId);
	printf("ClientViewSize: %I64X\r\n",ReceiveMsg.u3.ClientViewSize);


	unsigned long Offset = (TotalLength - DataLength) + DataInfoOffset;

	void* pData = ((char*)(&ReceiveMsg))+Offset;
	printf("Data: %s\r\n",(char*)pData);


	HANDLE hPort_remote = 0;
	if(Typ  == LPC_CONNECTION_REQUEST)
	{
		//Accept
		




		
		_CONTEXT_X* pContext = (_CONTEXT_X*)LocalAlloc(LMEM_ZEROINIT,sizeof(_CONTEXT_X));
		pContext->ClientId.UniqueProcess = ReceiveMsg.ClientId.UniqueProcess;
		pContext->ClientId.UniqueThread = ReceiveMsg.ClientId.UniqueThread;

		
		pContext->List1.Flink = &pContext->List1;
		pContext->List1.Blink = &pContext->List1;

		/*_LIST_ENTRY_* pEn = (_LIST_ENTRY_*)(pContext+0x40);
		pEn->FLink = pEn;
		pEn->BLink = pEn;*/

		pContext->unkx=0;
		//*(ulong*)(pContext+0x1C)=0;

		HANDLE hProcess = 0;
		_OBJECT_ATTRIBUTES ObjAttrX = {sizeof(ObjAttrX)};

		_CLIENT_ID ClientIdX={0};
		ClientIdX.UniqueProcess = ReceiveMsg.ClientId.UniqueProcess;
		ClientIdX.UniqueThread = ReceiveMsg.ClientId.UniqueThread;

		retValue = ZwOpenProcess(&hProcess,0x2000000,&ObjAttrX,&ClientIdX);
		if(retValue < 0)
		{
			printf("Error opening remote process, ret: %X\r\n",retValue);
			return -6;
		}

		printf("Remote Process Handle: %I64X\r\n",hProcess);

		//*(ulonglong*)(pContext+0x10) = (ulonglong)hProcess;
		pContext->hRemoteProcess = hProcess;


		/*_LIST_ENTRY_* pEnX = (_LIST_ENTRY_*)(pContext);
		pEnX->FLink = pEnX;
		pEnX->BLink = pEnX;*/

		pContext->List0.Flink = & pContext->List0;
		pContext->List0.Blink = & pContext->List0;


		_PORT_MESSAGE AcceptMsg ={0};
		AcceptMsg.MessageId = ReceiveMsg.MessageId;
		AcceptMsg.ClientId.UniqueProcess = ReceiveMsg.ClientId.UniqueProcess;
		AcceptMsg.ClientId.UniqueThread = ReceiveMsg.ClientId.UniqueThread;


		_REMOTE_PORT_VIEW  RemoteView = {0};
		RemoteView.Length=sizeof(_REMOTE_PORT_VIEW);

		retValue = ZwAcceptConnectPort
							(&hPort_remote,
							0,
							&AcceptMsg,
							true,
							0 /*		PPORT_VIEW ServerView*/,
							&RemoteView);

		printf("ZwAcceptConnectPort, ret: %X\r\n",retValue);
	}

	printf("================= Loooooop=====================>\r\n");
	_PORT_MESSAGE* pReceiveMsg = (_PORT_MESSAGE*)LocalAlloc(LMEM_ZEROINIT,MaxDataLength);

	while(1)
	{

			_PORT_MESSAGE Rpl={0};
			Rpl.ClientId.UniqueProcess = PID;
			Rpl.ClientId.UniqueThread = TID;

			memset(pReceiveMsg,0,MaxDataLength);
			//blocking call
			retValue = ZwReplyWaitReceivePort
								(hPort,
								&PortContext,
								&Rpl,
								pReceiveMsg);

			printf("ZwReplyWaitReceivePort, ret: %X\r\n",retValue);
			//Check	pReceiveMsg->u2.s2.Type
			unsigned short Typ = pReceiveMsg->u2.s2.Type;
			unsigned long TotalLength = pReceiveMsg->u1.s1.TotalLength;
			unsigned long DataLength = pReceiveMsg->u1.s1.DataLength;
			unsigned long DataInfoOffset = pReceiveMsg->u2.s2.DataInfoOffset;

	



			printf("Type: %X\r\n",Typ);
			printf("TotalLength: %X\r\n",TotalLength);
			printf("DataLength: %X\r\n",DataLength);
			printf("DataInfoOffset: %X\r\n",DataInfoOffset);
			printf("PID: %I64X\r\n",pReceiveMsg->ClientId.UniqueProcess);
			printf("TID: %I64X\r\n",pReceiveMsg->ClientId.UniqueThread);
			printf("MsgId: %X\r\n",pReceiveMsg->MessageId);
			printf("ClientViewSize: %I64X\r\n",pReceiveMsg->u3.ClientViewSize);

			unsigned long Offset = (TotalLength - DataLength) + DataInfoOffset;

			void* pData = ((char*)(pReceiveMsg))+Offset;
			printf("Data: %s\r\n",(char*)pData);


typedef enum _LPC_TYPE{
	LPC_NEW_MESSAGE, // A new message
	LPC_REQUEST, // A request message
	LPC_REPLY, // A reply to a request message
	LPC_DATAGRAM, //
	LPC_LOST_REPLY, //
	LPC_PORT_CLOSED, // Sent when port is deleted
	LPC_CLIENT_DIED, // Messages to thread termination ports
	LPC_EXCEPTION, // Messages to thread exception port
	LPC_DEBUG_EVENT, // Messages to thread debug port
	LPC_ERROR_EVENT, // Used by ZwRaiseHardError
	LPC_CONNECTION_REQUEST // Used by ZwConnectPort
}LPC_TYPE;

			printf("===============================>\r\n");
			if(Typ == LPC_NEW_MESSAGE)
			{
				printf("LPC_NEW_MESSAGE\r\n");
				//DIspatch
			}
			else if(Typ == LPC_REQUEST)
			{
				printf("LPC_REQUEST\r\n");
			}
			else if(Typ == LPC_REPLY)
			{
				printf("LPC_REPLY\r\n");
			}
			else if(Typ == LPC_DATAGRAM)
			{
				printf("LPC_DATAGRAM\r\n");
			}
			else if(Typ == LPC_LOST_REPLY)
			{
				printf("LPC_LOST_REPLY\r\n");
				//Disconnect
				break;
			}
			else if(Typ == LPC_PORT_CLOSED)
			{
				printf("LPC_PORT_CLOSED\r\n");
				//Disconnect
				break;
			}
			else if(Typ == LPC_CLIENT_DIED)
			{
				printf("LPC_CLIENT_DIED\r\n");
				//Disconnect
				break;
			}
			else if(Typ == LPC_EXCEPTION)
			{
				printf("LPC_EXCEPTION\r\n");
				//Disconnect
				break;
			}
			else if(Typ == LPC_DEBUG_EVENT)
			{
				printf("LPC_DEBUG_EVENT\r\n");
			}
			else if(Typ == LPC_ERROR_EVENT)
			{
				printf("LPC_ERROR_EVENT\r\n");
				//Disconnect
				break;
			}
			else if(Typ  == LPC_CONNECTION_REQUEST)
			{
				printf("LPC_CONNECTION_REQUEST\r\n");
			}
			Sleep(1000);
	}


	LocalFree(pReceiveMsg);



	return 0;

};


/*




*/