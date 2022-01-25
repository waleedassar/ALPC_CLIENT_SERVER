// ALPC_Client.cpp : Defines the entry point for the console application.
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

struct _ALPC_MESSAGE_ATTRIBUTES
{ 
	ulong AllocatedAttributes;
	ulong ValidAttributes;
};


extern "C"
{
	int ZwCreateSection(HANDLE* SectionHandle,
						ACCESS_MASK DesiredAccess,
						_OBJECT_ATTRIBUTES* ObjectAttributes,
						_LARGE_INTEGER* MaximumSize,
						ulonglong SectionPageProtection,
						ulonglong AllocationAttributes,
						HANDLE FileHandle);

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

	int ZwConnectPort
					(HANDLE* PortHandle,
					_UNICODE_STRING* PortName,
					_SECURITY_QUALITY_OF_SERVICE* SecurityQos, 
					_PORT_VIEW* ClientView, 

					_REMOTE_PORT_VIEW* ServerView,  
					ulonglong* MaxMessageLength, 
					void* ConnectionInformation, 
					ulonglong* ConnectionInformationLength);
	int ZwRequestWaitReplyPort
							(HANDLE PortHandle,
							_PORT_MESSAGE* RequestMessage,
							_PORT_MESSAGE* ReplyMessage);

	int ZwSecureConnectPort
							(HANDLE* PortHandle,
							_UNICODE_STRING* PortName,
							_SECURITY_QUALITY_OF_SERVICE* SecurityQos, 
							_PORT_VIEW* ClientView,
							PSID RequiredServerSid, 
							_REMOTE_PORT_VIEW ServerView,  
							ulonglong* MaxMessageLength, 
							void* ConnectionInformation, 
							ulonglong* ConnectionInformationLength);

	int ZwRequestPort(HANDLE PortHandle,  _PORT_MESSAGE* RequestMessage);


	int ZwAlpcConnectPort(HANDLE* PortHandle,
		_UNICODE_STRING* PortName,
		_OBJECT_ATTRIBUTES* ObjectAttributes,
		_ALPC_PORT_ATTRIBUTES* PortAttributes,
		ulonglong Flags,
		PSID RequiredServerSid,
		 _PORT_MESSAGE* ConnectionMessage, 
		 ulonglong* BufferLength,
		 _ALPC_MESSAGE_ATTRIBUTES* OutMessageAttributes,
		 _ALPC_MESSAGE_ATTRIBUTES* InMessageAttributes,
		 _LARGE_INTEGER* Timeout);
}




#define SECTION_EXTEND_SIZE 16
#define SECTION_MAP_READ 4
#define SECTION_MAP_WRITE 2
#define SECTION_QUERY 1
#define SECTION_ALL_ACCESS 0xf001f


#define SEC_COMMIT 0x8000000

int _tmainX(int argc, _TCHAR* argv[])
{
	printf("Current PID: %I64X\r\n",GetCurrentProcessId());
	printf("Current TID: %I64X\r\n",GetCurrentThreadId());

	HANDLE hSection = 0;
	_OBJECT_ATTRIBUTES ObjAttr={sizeof(ObjAttr)};

	ulonglong MaxSize=0x400000;


	int retValue = ZwCreateSection(&hSection,
		SECTION_MAP_READ|SECTION_MAP_WRITE,
		0 /*ObjAttr*/,
		(_LARGE_INTEGER*)(&MaxSize),
		PAGE_READWRITE,
		SEC_COMMIT,
		0);

	printf("ZwCreateSection, ret: %X\r\n",retValue);
	printf("hSection: %I64X\r\n",hSection);
	
	wchar_t* PortName = L"\\BaseNamedObjects\\Local\\portw";



	_UNICODE_STRING UniS = {0};
	UniS.Length = wcslen(PortName)*2;
	UniS.MaxLength = (UniS.Length)+2;
	UniS.Buffer = PortName;

	
	HANDLE hPort_remote = 0;

	_PORT_VIEW PortView = {sizeof(_PORT_VIEW)};
	PortView.SectionHandle=hSection;
	PortView.ViewSize=MaxSize;
	
	_REMOTE_PORT_VIEW RemotePortView = {sizeof(_REMOTE_PORT_VIEW)};

	ulonglong MaxMsgLength = 0;

	ulonglong ConnInfoLength = 0x30;
	void* ConnInfo = LocalAlloc(LMEM_ZEROINIT,ConnInfoLength);
	memset(ConnInfo,0x61,ConnInfoLength);

	retValue = ZwConnectPort(&hPort_remote,&UniS,0,&PortView,&RemotePortView,&MaxMsgLength,ConnInfo,&ConnInfoLength);
	if(retValue < 0)
	{
		char* Err = 0;
		if(retValue == 0xC0000034)
		{
			Err = "STATUS_OBJECT_NAME_NOT_FOUND";
		}
		else if(retValue == 0xC0000041)
		{
			Err = "STATUS_PORT_CONNECTION_REFUSED";
		}
		printf ("Error creating ALPC port, ret: %X, %s\r\n",retValue,Err);
		return -5;
	}

	printf("hPort: %X\r\n",hPort_remote);

	ulong c = 0;
	ulonglong i =0;
	while(1)
	{
		ulonglong MaxDataLength = 0xC0;
		MaxDataLength -= sizeof(_PORT_MESSAGE);

		_PORT_MESSAGE* pReq = (_PORT_MESSAGE*)LocalAlloc(LMEM_ZEROINIT,sizeof(_PORT_MESSAGE)+MaxDataLength);

		pReq->u1.s1.DataLength = MaxDataLength;
		pReq->u1.s1.TotalLength = MaxDataLength + sizeof(_PORT_MESSAGE);
		pReq->MessageId = i++;
		pReq->ClientId.UniqueProcess = GetCurrentProcessId();
		pReq->ClientId.UniqueThread = GetCurrentThreadId();
		pReq->u2.s2.DataInfoOffset = 0;

		memset( ((char*)pReq)+sizeof(_PORT_MESSAGE),0x61+c,MaxDataLength-1);
		c++;

		printf("===Attempt\r\n");
		_PORT_MESSAGE Repl ={0};
		retValue = ZwRequestWaitReplyPort
							(hPort_remote,
							pReq,
							0);
		
		if(retValue < 0)
		{
			printf("Error ZwRequestWaitReplyPort, ret: %X\r\n",retValue);
			return -7;
		}
		printf("===Done\r\n");
		Sleep(1000);
	}
	return 0;

};



//Send datagrams
int _tmain_datagram(int argc, _TCHAR* argv[])
{
	printf("Current PID: %I64X\r\n",GetCurrentProcessId());
	printf("Current TID: %I64X\r\n",GetCurrentThreadId());


	_OBJECT_ATTRIBUTES ObjAttr={sizeof(ObjAttr)};
	ObjAttr.Attributes = 0x40;




	
	wchar_t* PortName = L"\\BaseNamedObjects\\Local\\portw";



	_UNICODE_STRING UniS = {0};
	UniS.Length = wcslen(PortName)*2;
	UniS.MaxLength = (UniS.Length)+2;
	UniS.Buffer = PortName;

	



	HANDLE hPort_Remote = 0;

	_ALPC_PORT_ATTRIBUTES PortAtt = {0};

	int retValue = ZwAlpcConnectPort(&hPort_Remote,&UniS,&ObjAttr,0,0x20000 /*FLags*/,0,0,0,0,0,0);
	printf("ZwAlpcConnectPort, ret: %X\r\n",retValue);
	if(retValue < 0)
	{
		char* Err = 0;
		if(retValue == 0xC0000034)
		{
			Err = "STATUS_OBJECT_NAME_NOT_FOUND";
		}
		else if(retValue == 0xC0000041)
		{
			Err = "STATUS_PORT_CONNECTION_REFUSED";
		}
		printf ("Error creating ALPC port, ret: %X, %s\r\n",retValue,Err);
		return -5;
	}


	printf("hPort: %X\r\n",hPort_Remote);


	ulong c = 0;
	ulonglong i =0;
	while(1)
	{
		ulonglong TotalLength = 0x458;

		_PORT_MESSAGE* pReq = (_PORT_MESSAGE*)LocalAlloc(LMEM_ZEROINIT,TotalLength);


		ulonglong DataLength = TotalLength - sizeof(_PORT_MESSAGE);

		


		pReq->u1.s1.DataLength = 0;
		pReq->u1.s1.TotalLength = sizeof(_PORT_MESSAGE);
		//pReq->MessageId = i++;
		//pReq->ClientId.UniqueProcess = GetCurrentProcessId();
		//pReq->ClientId.UniqueThread = GetCurrentThreadId();
		pReq->u2.s2.DataInfoOffset = 0;

		//memset( ((char*)pReq)+sizeof(_PORT_MESSAGE),0x61+c,MaxDataLength-1);
		c++;

		printf("===Attempt\r\n");

		retValue = ZwRequestPort(hPort_Remote,pReq);
		printf("ZwRequestPort, ret: %X\r\n",retValue);
		if(retValue < 0)
		{
			printf("Error ZwRequestPort, ret: %X\r\n",retValue);
			return -7;
		}
		printf("===Done\r\n");
		Sleep(1000);
	}
	
	return 0;

};




int _tmain(int argc, _TCHAR* argv[])
{
	printf("Current PID: %I64X\r\n",GetCurrentProcessId());
	printf("Current TID: %I64X\r\n",GetCurrentThreadId());


	_OBJECT_ATTRIBUTES ObjAttr={sizeof(ObjAttr)};
	ObjAttr.Attributes = 0x40;




	
	wchar_t* PortName = L"\\BaseNamedObjects\\Local\\portw";



	_UNICODE_STRING UniS = {0};
	UniS.Length = wcslen(PortName)*2;
	UniS.MaxLength = (UniS.Length)+2;
	UniS.Buffer = PortName;

	



	HANDLE hPort_Remote = 0;

	_ALPC_PORT_ATTRIBUTES PortAtt = {0};

	int retValue = ZwAlpcConnectPort(&hPort_Remote,&UniS,&ObjAttr,0,0x20000 /*FLags*/,0,0,0,0,0,0);
	printf("ZwAlpcConnectPort, ret: %X\r\n",retValue);
	if(retValue < 0)
	{
		char* Err = 0;
		if(retValue == 0xC0000034)
		{
			Err = "STATUS_OBJECT_NAME_NOT_FOUND";
		}
		else if(retValue == 0xC0000041)
		{
			Err = "STATUS_PORT_CONNECTION_REFUSED";
		}
		printf ("Error creating ALPC port, ret: %X, %s\r\n",retValue,Err);
		return -5;
	}


	printf("hPort: %X\r\n",hPort_Remote);


	ulong c = 0;
	ulonglong i = 0;


	#define MaxMsgLen 0x200

	while(1)
	{


		ulonglong TotalLength = MaxMsgLen;
		_PORT_MESSAGE* pReq = (_PORT_MESSAGE*)VirtualAlloc(0,TotalLength,MEM_COMMIT,PAGE_READWRITE);
		_PORT_MESSAGE* pRpl = (_PORT_MESSAGE*)VirtualAlloc(0,TotalLength,MEM_COMMIT,PAGE_READWRITE);

		//pReq->u2.s2.Type = LPC_REQUEST;

		pReq->u1.s1.DataLength = TotalLength - sizeof(_PORT_MESSAGE);
		pReq->u1.s1.TotalLength = TotalLength;
		pReq->MessageId = 0;
		//pReq->ClientId.UniqueProcess = GetCurrentProcessId();
		//pReq->ClientId.UniqueThread = GetCurrentThreadId();
		pReq->u2.s2.DataInfoOffset = 0 ;

		memset( ((char*)pReq)+sizeof(_PORT_MESSAGE)+(pReq->u2.s2.DataInfoOffset),
				0x61+c,
				TotalLength - sizeof(_PORT_MESSAGE)-1);


		c++;

		printf("===Attempt\r\n");

		retValue = ZwRequestWaitReplyPort(hPort_Remote,pReq,pRpl);
		printf("ZwRequestWaitReplyPort, ret: %X\r\n",retValue);
		if(retValue < 0)
		{
			printf("Error ZwRequestWaitReplyPort, ret: %X\r\n",retValue);
			return -7;
		}
		printf("===Done\r\n");

		VirtualFree(pReq,0,MEM_RELEASE);
		Sleep(1000);
	}
	
	return 0;

};