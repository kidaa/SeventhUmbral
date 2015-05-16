#include <stdio.h>
#include <stdlib.h>
#include "SocketDef.h"
#include "LobbyServer.h"
#include "GameServer.h"
#include "GlobalData.h"
#include "AppConfig.h"
#include "Log.h"
#ifdef __unix__
#include <sys/ioctl.h>
#endif

int main(int argc, char** argv)
{
#ifdef WIN32
	WSADATA wsaData;
	if(WSAStartup(WINSOCK_VERSION, &wsaData))
	{
		printf("Failed to initialize WSA.\r\n");
		return -1;
	}
#endif

#ifdef __unix__

	if(fork() != 0)
	{
		exit(0);
	}

	setpgrp(0, getpid());
	int fd = open("/dev/tty", O_RDWR);
	if(fd >= 0)
	{
		ioctl(fd, TIOCNOTTY, 0);
		close(fd);
	}

#endif

	if(!CAppConfig::GetInstance().IsConfigAvailable())
	{
		auto configPath = CAppConfig::GetInstance().GetBasePath();
		CLog::GetInstance().LogError("Global", "Config file not available. Make sure there is a 'config.xml' file in '%s'.", configPath.string().c_str());
	}

	CGlobalData::GetInstance().Prepare();

	CLobbyServer lobbyServer;
	lobbyServer.Start();

	CGameServer gameServer;
	gameServer.Start();

	while(1) std::this_thread::sleep_for(std::chrono::milliseconds(10000));

#ifdef WIN32
	WSACleanup();
#endif
}
