//#ifndef _WEB_CONN_CLIENT_H_
//#define _WEB_CONN_CLIENT_H_
//
//#include "interface.h"
//#include "ByteBuffer.h"
//#include "PolarSSL++/AesCfb128Decryptor.h"
//#include "PolarSSL++/AesCfb128Encryptor.h"
//#include "Globals.h"
//#include "OSSupport/CriticalSection.h"
//#include <iostream>
//
//class WebConnClient {
//
//	friend class cPacketizer;
//
//public:
//	WebConnClient();
//	virtual ~WebConnClient();
//
//public:
//	void RegisterObserver(ConnectionClientObserver* callback);
//	bool is_connected() const;
//
//	bool Init(const std::string & a_Server, const int &a_Port, const std::string &a_UserName);
//	bool UnInit();
//
//	UInt32 GetID();
//	std::string GetChannel();
//
//	void ConnectToServer();
//	bool SendToChannel(const std::string &room, const UInt32 &a_ClientID, const std::string &a_Msg);
//	void JoinChannel(const std::string &server, const std::string &room);
//	void LeaveChannel(const std::string &room);
//	bool SendHangUp(int peer_id);
//	void SignOut();
//
//	LRESULT OnNetwork(WPARAM wParam, LPARAM lParam);
//
//protected:
//	void SendData(const char *a_Data, size_t a_Size);
//	void DataReceived(const char *a_Data, size_t a_Size);
//	BOOL SetTimeout(int nTime, BOOL bRecv);
//	void OnConnect(SOCKET sock);
//	bool OnRead(SOCKET sock);
//	void OnClose(SOCKET sock);
//	void AddReceivedData(const char * a_Data, size_t a_Size);
//	bool HandlePacket(cByteBuffer &a_ByteBuffer, UInt32 a_PacketType);
//protected:
//
//	ConnectionClientObserver* callback_;
//
//	SOCKET			sock_;
//	std::string			server_;
//	short			port_;
//
//	/** Buffer for the received data */
//	cByteBuffer m_ReceivedData;
//
//	//cCriticalSection m_CSPacket;
//	std::mutex mux;
//
//	/** Buffer for composing the outgoing packets, through cPacketizer */
//	cByteBuffer m_OutPacketBuffer;
//
//	/** State of the protocol. 1 = status, 2 = login, 3 = work */
//	UInt32 m_State;
//
//	bool				m_IsEncrypted;
//	bool				m_bConnect;
//	UInt32				m_ClientID;
//
//	cAesCfb128Decryptor *m_Decryptor;
//	cAesCfb128Encryptor *m_Encryptor;
//
//	std::string				m_rUserName;
//	std::string				m_rChannel;
//};
//
//#endif