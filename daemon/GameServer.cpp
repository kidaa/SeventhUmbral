#include "SocketDef.h"
#include <assert.h>
#include <math.h>
#include "GameServer.h"
#include "PacketUtils.h"
#include "SocketUtils.h"
#include "GameServerPlayer.h"
#include "GameServer_Login.h"
#include "Log.h"

#define LOG_NAME		("GameServer")

static void ClientThreadProc(SOCKET clientSocket, const sockaddr_in& clientSocketAddress, int clientId)
{
#ifdef WIN32
	u_long notBlockingMode = 1;
	ioctlsocket(clientSocket, FIONBIO, &notBlockingMode);
#else
	int flags = fcntl(clientSocket, F_GETFL, 0);
	fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
#endif

	CLog::GetInstance().LogMessage(LOG_NAME, "Received connection from %s.", GetSocketIpAddressString(clientSocketAddress).c_str());

	if(clientId == 0)
	{
		CGameServerPlayer player(clientSocket);
		while(player.IsConnected())
		{
			player.Update();
			std::this_thread::sleep_for(std::chrono::milliseconds(16));
		}
	}
	else
	{
		{
			std::vector<uint8> outgoingPacket(std::begin(g_client1_login1), std::end(g_client1_login1));
			int sent = send(clientSocket, reinterpret_cast<const char*>(outgoingPacket.data()), outgoingPacket.size(), 0);
			assert(sent == outgoingPacket.size());
		}

		{
			std::vector<uint8> outgoingPacket(std::begin(g_client1_login2), std::end(g_client1_login2));
			int sent = send(clientSocket, reinterpret_cast<const char*>(outgoingPacket.data()), outgoingPacket.size(), 0);
			assert(sent == outgoingPacket.size());
		}

		while(1)
		{
			static const unsigned int maxPacketSize = 0x10000;
			uint8 incomingPacket[maxPacketSize];
			int read = recv(clientSocket, reinterpret_cast<char*>(incomingPacket), maxPacketSize, 0);
			if(read == 0)
			{
				//Client disconnected
				CLog::GetInstance().LogMessage(LOG_NAME, "Client disconnected.");
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	closesocket(clientSocket);
}

CGameServer::CGameServer()
{

}

CGameServer::~CGameServer()
{

}

void CGameServer::Start()
{
	std::thread serverThread(std::bind(&CGameServer::ServerThreadProc, this));
	m_serverThread = std::move(serverThread);
}

void CGameServer::ServerThreadProc()
{
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	{
		int reuseOptionValue = 1;
		int result = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuseOptionValue), sizeof(reuseOptionValue));
		assert(result == 0);
	}

	sockaddr_in service;
	service.sin_family			= AF_INET;
	service.sin_addr.s_addr		= htonl(INADDR_ANY);
	service.sin_port			= htons(GAME_SERVER_PORT);
	if(bind(listenSocket, reinterpret_cast<sockaddr*>(&service), sizeof(sockaddr_in)))
	{
		CLog::GetInstance().LogError(LOG_NAME, "Failed to bind socket.");
		return;
	}

	if(listen(listenSocket, SOMAXCONN))
	{
		CLog::GetInstance().LogError(LOG_NAME, "Failed to listen on socket.");
		return;
	}

	CLog::GetInstance().LogMessage(LOG_NAME, "Game server started.");

	int clientId = 0;

	while(1)
	{
		sockaddr_in incomingAddr;
		socklen_t incomingAddrSize = sizeof(sockaddr_in);
		SOCKET incomingSocket = accept(listenSocket, reinterpret_cast<sockaddr*>(&incomingAddr), &incomingAddrSize);
		std::thread clientThread(std::bind(&ClientThreadProc, incomingSocket, incomingAddr, clientId & 1));
		clientThread.detach();
		clientId++;
	}

	closesocket(listenSocket);
}
