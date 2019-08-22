#include "stdafx.h"
#include "ConnectionClient.h"
#include "Packetizer.h"
#include "defaults.h"
#include "common.h"
#include <httpext.h>
#include <windef.h>
#include <nb30.h>

#include <ErrorCode.h>

#pragma comment(lib,"netapi32")
#include "PolarSSL++/CryptoKey.h"

#include "setdebugnew.h"


#define HANDLE_READ(ByteBuf, Proc, Type, Var) \
	Type Var; \
	if (!ByteBuf.Proc(Var))\
					{\
		return;\
					}


ConnectionClient::ConnectionClient()
	: callback_(nullptr)
	, sock_(0)
	, m_State(1)
	, m_ClientID(0)
	, m_bConnect(false)
	, m_IsEncrypted(false)
	, m_Decryptor(nullptr)
	, m_Encryptor(nullptr)
	, m_ReceivedData(16 KiB)
	, m_OutPacketBuffer(16 KiB)
{
}


ConnectionClient::~ConnectionClient()	
{
	if (m_Encryptor)
	{
		delete m_Encryptor;
		m_Encryptor = nullptr;
	}
	if (m_Decryptor)
	{
		delete m_Decryptor;
		m_Decryptor = nullptr;
	}
}

void ConnectionClient::RegisterObserver(ConnectionClientObserver* callback)
{
	ASSERT(callback);
	callback_ = callback;
}

bool ConnectionClient::is_connected() const
{
	return m_bConnect;
}

bool ConnectionClient::Init(const AString& a_Server, const int &a_Port, const AString &a_UserName)
{
	//��ʼ���׽���
	WSADATA wsa = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		return false;
	}

	server_ = a_Server;
	port_ = a_Port;
	m_rUserName = a_UserName;

	return true;
}

bool ConnectionClient::UnInit()
{
	WSACleanup();
	
	return true;
}

UInt32 ConnectionClient::GetID()
{
	return m_ClientID;
}

AString ConnectionClient::GetChannel()
{
	return m_rChannel;
}

void ConnectionClient::SignOut()
{
	closesocket(sock_);
	sock_ = 0;
	m_ClientID = 0;
	m_rChannel = "";

	m_ReceivedData.ClearAll();
	m_OutPacketBuffer.ClearAll();

	if (m_Encryptor)
	{
		delete m_Encryptor;
		m_Encryptor = nullptr;
	}
	if (m_Decryptor)
	{
		delete m_Decryptor;
		m_Decryptor = nullptr;
	}

	m_State = 1;
	m_IsEncrypted = false;
	m_bConnect = false;
}

void ConnectionClient::ConnectToServer()
{
	//�����׽���  
	if ((sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
	{
		callback_->PostNotification("�����׽���ʧ��");
		return;
	}
	struct hostent *hptr;
	if ((hptr = gethostbyname(server_.c_str())) == NULL)
	{
		return;
	}
	
	//���÷�������Ϣ
	sockaddr_in serverAddr;	
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	memcpy(&serverAddr.sin_addr.s_addr, hptr->h_addr, hptr->h_length);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port_);
	
	char *serverIp = inet_ntoa(serverAddr.sin_addr);

	//�첽��Ϣ
	if (WSAAsyncSelect(sock_, callback_->handle(), WM_NETWORK, FD_CONNECT | FD_READ | FD_CLOSE) == SOCKET_ERROR)
	{
		callback_->PostNotification("ע�������¼�ʧ��");
		UnInit();
	}

	SetTimeout(3000, FALSE);//���ͳ�ʱ3��
	SetTimeout(1000, TRUE); //���ճ�ʱ3��


	//��ֹNagle�㷨���������ӵ������
	const char opt = 1;
	int	nErr = setsockopt(sock_, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(char));
	if (nErr == -1)
	{
		callback_->PostNotification("�������" + ::GetLastError());
	}

	connect(sock_, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
}

LRESULT ConnectionClient::OnNetwork(WPARAM wParam, LPARAM lParam)
{
	SOCKET sock = (SOCKET)wParam;
	WORD netEvent = WSAGETSELECTEVENT(lParam);
	WORD error = WSAGETSELECTERROR(lParam);
	if (error != 0)
	{
		if (error == WSAECONNREFUSED)
		{
			OnClose(sock);
			callback_->PostNotification("���ڱ�Ŀ������ܾ��������޷�����");
		}
		else if (error == WSAETIMEDOUT)
		{
			OnClose(sock);
			callback_->PostNotification("���ӳ�ʱ");
		}
		else if (error == WSAEHOSTUNREACH)
		{
			OnClose(sock);
			callback_->PostNotification("Ӧ�ó�����ͼ����һ�����ɵִ������");
		}
		return -1;
	}
	switch (netEvent)
	{
	case FD_CONNECT:
		OnConnect(sock);
		break;
	case FD_READ:
		OnRead(sock);
		break;
	case FD_CLOSE:
		OnClose(sock);
		break;
	default:
		break;
	}
	return S_OK;
}

BOOL ConnectionClient::SetTimeout(int nTime, BOOL bRecv)
{
	int ret = setsockopt(sock_, SOL_SOCKET, bRecv ? SO_RCVTIMEO : SO_SNDTIMEO, (char*)&nTime, sizeof(nTime));
	if (ret == -1)
	{
		printf("setsockopt() error is %d\n", ::GetLastError());
	}
	return SOCKET_ERROR != ret;
}

void ConnectionClient::OnConnect(SOCKET sock)
{
	m_bConnect = true;

	cPacketizer pkg(*this,0x00);// Packet type - StatusRequest
	pkg.WriteString("mcu");
	pkg.WriteString(SERVER);
	pkg.WriteBEInt32(PORT);
	pkg.WriteBEInt32(2);// NextState - loginStart

}

bool ConnectionClient::OnRead(SOCKET sock)
{
	char Buffer[64 KiB] = {0};
	int res = static_cast<int>(recv(sock, Buffer, sizeof(Buffer), 0));  // recv returns int on windows, ssize_t on linux
	if (res <= 0)
	{
		LOG("Server closed the socket: %d; %d; aborting connection", res, WSAGetLastError());
		return false;
	}

	DataReceived(Buffer, res);

	return true;
}

void ConnectionClient::OnClose(SOCKET sock)
{
	m_bConnect = false;
	callback_->OnDisconnected();
}

void ConnectionClient::SendData(const char * a_Data, size_t a_Size)
{
	if (m_IsEncrypted)
	{
		Byte Encrypted[8192];  // Larger buffer, we may be sending lots of data (chunks)
		while (a_Size > 0)
		{
			size_t NumBytes = (a_Size > sizeof(Encrypted)) ? sizeof(Encrypted) : a_Size;
			m_Encryptor->ProcessData(Encrypted, (Byte *)a_Data, NumBytes);
			::send(sock_, (const char *)Encrypted, NumBytes, 0);
			a_Size -= NumBytes;
			a_Data += NumBytes;
		}
	}
	else
	{
		::send(sock_, a_Data, a_Size, 0);
	}
}

void ConnectionClient::DataReceived(const char * a_Data, size_t a_Size)
{
	if (m_IsEncrypted)
	{
		Byte Decrypted[512];
		while (a_Size > 0)
		{
			size_t NumBytes = (a_Size > sizeof(Decrypted)) ? sizeof(Decrypted) : a_Size;
			m_Decryptor->ProcessData(Decrypted, (Byte *)a_Data, NumBytes);
			AddReceivedData((const char *)Decrypted, NumBytes);
			a_Size -= NumBytes;
			a_Data += NumBytes;
		}
	}
	else
	{
		AddReceivedData(a_Data, a_Size);
	}
}

void ConnectionClient::AddReceivedData(const char * a_Data, size_t a_Size)
{
	if (!m_ReceivedData.Write(a_Data, a_Size))
	{
		// Too much data in the incoming queue, report to caller:
		callback_->PacketBufferFull();
		return;
	}

	// Handle all complete packets:
	for (;;)
	{
		UInt32 PacketLen;
		if (!m_ReceivedData.ReadVarInt(PacketLen))
		{
			// Not enough data
			m_ReceivedData.ResetRead();
			break;
		}
		if (!m_ReceivedData.CanReadBytes(PacketLen))
		{
			// The full packet hasn't been received yet
			m_ReceivedData.ResetRead();
			break;
		}
		cByteBuffer bb(PacketLen + 1);
		VERIFY(m_ReceivedData.ReadToByteBuffer(bb, static_cast<size_t>(PacketLen)));
		m_ReceivedData.CommitRead();

		UInt32 PacketType;
		if (!bb.ReadVarInt(PacketType))
		{
			// Not enough data
			break;
		}

		// Write one NUL extra, so that we can detect over-reads
		bb.Write("\0", 1);


		if (!HandlePacket(bb, PacketType))
		{
			// Unknown packet, already been reported, but without the length. Log the length here:
			LOGWARNING("Unhandled packet: type 0x%x, state %d, length %u", PacketType, m_State, PacketLen);

			return;
		}

		if (bb.GetReadableSpace() != 1)
		{
			// Read more or less than packet length, report as error
			LOGWARNING("Protocol 1.7: Wrong number of bytes read for packet 0x%x, state %d. Read " SIZE_T_FMT " bytes, packet contained %u bytes",
				PacketType, m_State, bb.GetUsedSpace() - bb.GetReadableSpace(), PacketLen
				);

			ASSERT(!"Read wrong number of bytes!");
			callback_->PacketError(PacketType);
		}
	}  // for (ever)
}

bool ConnectionClient::SendToChannel(const AString &room, const UInt32 &a_ClientID, const AString &a_Msg)
{
	cPacketizer pkg(*this, 0x02);//media packet
	pkg.WriteString(room);
	pkg.WriteVarInt32(a_ClientID);
	pkg.WriteString(a_Msg);
	return true;
}

void ConnectionClient::JoinChannel(const AString &server, const AString &room)
{
	m_rChannel = room;

	if (!m_bConnect)
	{
		server_ = server;
		ConnectToServer();
	}
}

void ConnectionClient::LeaveChannel(const AString &room)
{
	cPacketizer pkg(*this, 0x09);//
	pkg.WriteString(room);
}

bool ConnectionClient::SendHangUp(int peer_id)
{
	return true;
}

bool ConnectionClient::HandlePacket(cByteBuffer & a_ByteBuffer, UInt32 a_PacketType)
{
	switch (m_State)
	{
	case 1:
	{
		// Status
		switch (a_PacketType)
		{
 		case 0x00: HandlePacketStatusRequest(a_ByteBuffer); return true;
// 		case 0x01: HandlePacketStatusPing(a_ByteBuffer); return true;
		}
		break;
	}

	case 2:
	{
		// Login
		switch (a_PacketType)
		{			
		case 0x00: HandlePacketLoginDisconnect(a_ByteBuffer); return true;
		case 0x01: HandlePacketLoginEncryptionKeyRequest(a_ByteBuffer); return true;
		case 0x02: HandlePacketLoginAllow(a_ByteBuffer); return true;
		case 0x03: HandlePacketLoginSuccess(a_ByteBuffer); return true;
		}
		break;
	}

	case 3:
	{
		// Game
		switch (a_PacketType)
		{
		case 0x00: HandlePacketKeepAlive(a_ByteBuffer); return true;
		case 0x01: HandlePacketRoomJoin(a_ByteBuffer); return true;
		case 0x02: HandlePacketChannelMsg(a_ByteBuffer); return true;
		case 0x09: HandlePacketRoomLeft(a_ByteBuffer); return true;
		case 0x40: HandlePacketErrorCode(a_ByteBuffer); return true;
		}
		break;
	}
	default:
	{
		// Received a packet in an unknown state, report:
		LOGWARNING("Received a packet in an unknown protocol state %d. Ignoring further packets.", m_State);

		// Cannot kick the client - we don't know this state and thus the packet number for the kick packet

		// Switch to a state when all further packets are silently ignored:
		m_State = 255;
		return false;
	}
	case 255:
	{
		// This is the state used for "not processing packets anymore" when we receive a bad packet from a client.
		// Do not output anything (the caller will do that for us), just return failure
		return false;
	}
	}  // switch (m_State)

	// Unknown packet type, report to the ClientHandle:
	callback_->PacketUnknown(a_PacketType);
	return false;
}

void ConnectionClient::HandlePacketStatusRequest(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadBEInt32, int, state);
	HANDLE_READ(a_ByteBuffer, ReadBool, bool, zip);
	HANDLE_READ(a_ByteBuffer, ReadBool, bool, encrypt);
	HANDLE_READ(a_ByteBuffer, ReadVarUTF8String, AString, strIP);

	if (state>3 || state <1)
	{
		return;
	}

	m_State = state;
	

	cPacketizer pkg(*this, 0x00);// Packet type - loginStart
}

void ConnectionClient::HandlePacketLoginDisconnect(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadBEInt32, int, a_Reason);
	HandleErrorCode(a_Reason);
}

void ConnectionClient::HandlePacketLoginEncryptionKeyRequest(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadVarUTF8String, AString, serverID);
	HANDLE_READ(a_ByteBuffer, ReadBEInt16, Int16, keyLen);
	char *bufer = new char[keyLen];
	memset(bufer, 0, keyLen);
	a_ByteBuffer.ReadBuf(bufer, keyLen);
	HANDLE_READ(a_ByteBuffer, ReadBEInt16, Int16, NonceLen);
	
	HANDLE_READ(a_ByteBuffer, ReadBEInt32, Int32, Nonce);

	if (NonceLen != 4)
	{
		return;
	}

	AString PublicKey(bufer, keyLen);

	SendEncryptionKeyResponse(PublicKey, Nonce);

	delete[]bufer;
}

void ConnectionClient::HandlePacketLoginAllow(cByteBuffer & a_ByteBuffer)
{
	cPacketizer pkg(*this, 0x02);// Packet type - loginInfo
	pkg.WriteString(m_rUserName);
}


void ConnectionClient::HandlePacketLoginSuccess(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadVarInt32, UInt32, clientID);

	m_ClientID = clientID;

	m_State = 3;
	callback_->OnSignedIn();

	cPacketizer pkg(*this, 0x01);//
	pkg.WriteString(m_rChannel);
}


void ConnectionClient::HandlePacketKeepAlive(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadVarInt32, UInt32, KeepAliveID);
	cPacketizer pkg(*this, 0x00);// Packet type - KeepAlive
	pkg.WriteVarInt32(KeepAliveID);
}

void ConnectionClient::HandlePacketRoomJoin(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadVarUTF8String, AString, channel);
	HANDLE_READ(a_ByteBuffer, ReadVarInt32, UInt32, ret);
	if (ret)
	{
		//�������������room
		callback_->OnJoinedChannel(channel);
	}
}

void ConnectionClient::HandlePacketChannelMsg(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadVarInt32, UInt32, fromID);
	HANDLE_READ(a_ByteBuffer, ReadVarUTF8String, AString, channel);
	HANDLE_READ(a_ByteBuffer, ReadVarUTF8String, AString, msg);

	callback_->OnMessageFromChannel(channel, fromID, msg);
}

void ConnectionClient::HandlePacketRoomLeft(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadVarUTF8String, AString, channel);
	HANDLE_READ(a_ByteBuffer, ReadVarInt32, UInt32, ret);

	if (ret)
	{
		//�������������room
		callback_->OnLeaveChannel(channel);
	}
	m_rChannel = "";
}

void ConnectionClient::HandlePacketErrorCode(cByteBuffer & a_ByteBuffer)
{
	HANDLE_READ(a_ByteBuffer, ReadBEInt32, int, a_Reason);
	HandleErrorCode(a_Reason);
}

AString& ReplaceAll(AString& str, const AString& old_value, const AString& new_value)
{
	while (true)
	{
		AString::size_type   pos(0);
		if ((pos = str.find(old_value)) != AString::npos)
			str.replace(pos, old_value.length(), new_value);
		else
			break;
	}
	return str;
}

void ConnectionClient::SendEncryptionKeyResponse(const string & a_PublicKey, const UInt32 & a_Nonce)
{
	// Generate the shared secret and encrypt using the server's public key
	Byte SharedSecret[16];
	Byte EncryptedSecret[128];

	::srand((UInt32)callback_->handle());

	for (auto i = 0; i<sizeof(SharedSecret);++i)
	{
		SharedSecret[i] = rand() % 255;
	}	

	//memset(SharedSecret, 0, sizeof(SharedSecret));

	cCryptoKey PubKey(a_PublicKey);
	int res = PubKey.Encrypt(SharedSecret, sizeof(SharedSecret), EncryptedSecret, sizeof(EncryptedSecret));
	if (res < 0)
	{
		LOG("Shared secret encryption failed: %d (0x%x)", res, res);
		return;
	}
	m_Encryptor = new cAesCfb128Encryptor;
	m_Decryptor = new cAesCfb128Decryptor;

	m_Encryptor->Init(SharedSecret, SharedSecret);
	m_Decryptor->Init(SharedSecret, SharedSecret);

	// Encrypt the nonce:
	Byte EncryptedNonce[128];
	res = PubKey.Encrypt((const Byte *)&a_Nonce, sizeof(UInt32), EncryptedNonce, sizeof(EncryptedNonce));
	if (res < 0)
	{
		LOG("Nonce encryption failed: %d (0x%x)", res, res);
		return;
	}

	// Send the packet to the server:
	LOG("Sending PACKET_ENCRYPTION_KEY_RESPONSE to the SERVER");

	{
		cPacketizer pkg(*this, 0x01);// Packet type - LoginEncryptionResponse
		pkg.WriteBEUInt16(static_cast<UInt16>(sizeof(EncryptedSecret)));
		pkg.WriteBuf((char*)EncryptedSecret, sizeof(EncryptedSecret));
		pkg.WriteBEUInt16(static_cast<UInt16>(sizeof(EncryptedNonce)));
		pkg.WriteBuf((char*)EncryptedNonce, sizeof(EncryptedNonce));
	}
	

	m_IsEncrypted = true;
}

void ConnectionClient::HandleErrorCode(const int &a_Code)
{
	CString strReason;
	switch (a_Code)
	{
	case  ERROR_CODE_ACCOUNT_NOT_MATCH:
		strReason = _T("�û������벻ƥ�䣡");
		break;
	case ERROR_CODE_ACCOUNT_NOT_EXIST:
		strReason = _T("�û��������ڣ�");
		break;
	case ERROR_CODE_PACKET_ERROR:
		strReason = _T("Э�����");
		break;
	case ERROR_CODE_PACKET_UNKONWN:
		strReason = _T("δ֪Э�飡");
		break;
	case ERROR_CODE_SERVER_BUSY:
		strReason = _T("��������æ��");
		break;
	case ERROR_CODE_CLIENT_TIMEOUT:
		strReason = _T("Ŷ���������ʱ���Ժ����ԣ�");
		break;
	case  ERROR_CODE_ACCOUNT_ALREADY_LOGIN:
		strReason = _T("��ͬ�˺��Ѿ���½��");
		break;
	case ERROR_CODE_ACCOUNT_OTHER_LOGIN:
		strReason = _T("��ͬ�˺�����ص�½��");
		break;
	case ERROR_CODE_SERVER_SHUTDOWN:
		strReason = _T("�������Ѿ��رգ�");
		break;
	case ERROR_CODE_BAD_APP:
		strReason = _T("�Ƿ��ͻ��ˣ�");
		break;
	case  ERROR_CODE_INVALID_ENCKEYLENGTH:
		strReason = _T("��Ч�����ݳ��ȣ�");
		break;
	case  ERROR_CODE_INVALID_ENCNONCELENGTH:
		strReason = _T("��Ч�����ݳ��ȣ�h");
		break;
	case  ERROR_CODE_HACKED_CLIENT:
		strReason = _T("�ڿͷǷ��ͻ��ˣ�");
		break;
	default:
		break;
	}
	callback_->OnLoginError();
	callback_->PostNotification(strReason,RGB(255,0,255));
}
