#include <iostream>
#include <string>
#include <windows.h>
#include <commctrl.h>
#include <stdint.h>
#include "Math.h"
using namespace std;


/*
First project attempting to read game memory to implement features such as:

Make CLASS
GET ENEMY ADRESSES

AIMBOT
ESP
WRITING VALUES
*/
// Found offsets from ASSAULT_CUBE base address.
struct AC_PLAYER_OFFSETS
{
	UINT32 playerXPos = 0x04;
	UINT32 playerHeadPos = 0x08;
	UINT32 playerYPos = 0x3C;
	UINT32 playerZPos = 0x0C; 
	UINT32 playerXmouse = 0x44;
	UINT32 playerYmouse = 0x40;
	UINT32 playerHealth = 0xF8;
	UINT32 playerArmor = 0xFC;
	UINT32 playerTeam = 0x32C;
	UINT32 assaultRifleAmmo = 0x150;
};

struct PlayerDataVector
{
	int playerHealth;
	float playerXPos;
	float playerYPos;
	float playerHeadPos;
	float playerZPos;
	float playerXmouse;
	float playerYmouse;
};


class Bypass {
public:
	// Constructor & destructor
	Bypass();
	~Bypass();

	// Public methods
	bool Attach(DWORD dwPid);
	bool Read(uintptr_t lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead = NULL);
	bool Write(uintptr_t lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten = NULL);

private:
	HANDLE m_hProcess = NULL;
};

Bypass::Bypass() {}
Bypass::~Bypass() {
	if (m_hProcess != NULL) CloseHandle(m_hProcess);
}
bool Bypass::Attach(DWORD dwPid) {
	m_hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, dwPid);
	if (m_hProcess != NULL) return true; return false;
}
bool Bypass::Read(uintptr_t lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesRead) {
	if (ReadProcessMemory(m_hProcess, (LPCVOID)lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead)) return true; return false;
}
bool Bypass::Write(uintptr_t lpBaseAddress, void* lpBuffer, SIZE_T nSize, SIZE_T* lpNumberOfBytesWritten) {
	if (WriteProcessMemory(m_hProcess, (LPVOID)lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten)) return true; return false;
}

float Get3dDistance(PlayerDataVector source, PlayerDataVector dest);
bool ObtainPlayerData(Bypass* hook, uintptr_t bPointer, AC_PLAYER_OFFSETS pOffset, PlayerDataVector *player);
void PrintOutVariables(PlayerDataVector player,int playerNum);
float GetPitch(PlayerDataVector enemy, PlayerDataVector player);
float GetYaw(PlayerDataVector enemy, PlayerDataVector player);


// Takes a base address of player, and applies values with pointer offsets.
bool ObtainPlayerData(Bypass* hook, uintptr_t bPointer, AC_PLAYER_OFFSETS pOffset, PlayerDataVector *player)
{
	bool RPM;
	RPM = hook->Read((bPointer+pOffset.playerHealth),&player->playerHealth,sizeof(player->playerHealth));
	RPM = hook->Read((bPointer+pOffset.playerXmouse),&player->playerXmouse,sizeof(player->playerXmouse));
	RPM = hook->Read((bPointer+pOffset.playerYmouse),&player->playerYmouse,sizeof(player->playerYmouse));
	RPM = hook->Read((bPointer+pOffset.playerXPos),&player->playerXPos,sizeof(player->playerXPos));
	RPM = hook->Read((bPointer+pOffset.playerYPos),&player->playerYPos,sizeof(player->playerYPos));
	RPM = hook->Read((bPointer+pOffset.playerZPos),&player->playerZPos,sizeof(player->playerZPos));
	RPM = hook->Read((bPointer+pOffset.playerHeadPos),&player->playerHeadPos,sizeof(player->playerHeadPos));
	return RPM;
}

void PrintOutVariables(PlayerDataVector player,int playerNum)
{

	printf("PLAYER %d \n",playerNum);
	printf("Health: %u \n", player.playerHealth);
	printf("Mouse X: %.6f  \n", player.playerXmouse);
	printf("Mouse Y: %.6f \n",player.playerYmouse);
	printf("X Position: %.6f \n",player.playerXPos);
	printf("Y Position: %.6f \n",player.playerYPos);
	printf("Z Position: %.6f \n",player.playerZPos);
	printf("Head Position: %.6f \n",player.playerHeadPos);
	printf("----------------------------------\n");

}


float GetPitch(PlayerDataVector enemy, PlayerDataVector player)
{
	float pitchX = (float)atan2(enemy.playerZPos - player.playerZPos, Get3dDistance(enemy,player)) * 180 / 3.1459265f;
	return pitchX;

}
float GetYaw(PlayerDataVector enemy, PlayerDataVector player)
{
	float yawY = -(float)atan2(enemy.playerXPos - player.playerXPos, enemy.playerHeadPos - player.playerHeadPos ) / 3.1459265f * 180 + 180;
	return yawY;

}

float Get3dDistance(PlayerDataVector source, PlayerDataVector dest)
{
	return (float)(sqrt
		(
	pow((source.playerXPos - dest.playerXPos),2) +
	pow((source.playerYPos - dest.playerYPos),2) +
	pow((source.playerHeadPos - dest.playerHeadPos),2)
));
}


int main()
{
	/*Aimbot variables*/
	float _PI = 3.1459265f;
	bool enemyFocused = false;
	int focusTarget = -1;
	/*Memory address and offset variables*/
	AC_PLAYER_OFFSETS player;
	PlayerDataVector current;
	PlayerDataVector enemyOne;
	// Static base adress 0x400000 + 109B74
	uintptr_t BASE_ADDRESS = 0x509B74;
	uintptr_t BASE_POINTER;
	uintptr_t BASE_ADDRESS_ENEMY1 = 0x510D90;
	uintptr_t BASE_POINTER_ENEMY1;
	DWORD pid=0;
	DWORD BASE_DEREFERENCE = 0x0;
	DWORD BASE_DEREFERENCE_EM1 = 0x0;

	cout << "Searching for Gamewindow. . ."<< endl;
	HWND hwnd = FindWindow(NULL, "AssaultCube");
	if(hwnd == NULL)
	{
		cout << "Cannot locate window. . .\nExiting.";
		exit(-1);
	}
	cout << "Obtaining Thread PID ... "<< endl;
	GetWindowThreadProcessId(hwnd,&pid);
	if(pid==0)
	{
		cout << "Cannot get PID. . . Exiting.";
		exit(-1);
	}
	cout << "Attempting to open handle on process: " << pid << " . . . " << endl;
	Bypass* bypass = new Bypass();
	if (!bypass->Attach(pid))
	{
		cout << "OpenProcess failed. GetLastError = " << dec << GetLastError() << endl;
		delete bypass;
		exit(-1);
	}

	if(!bypass->Read(BASE_ADDRESS,&BASE_DEREFERENCE,sizeof(BASE_DEREFERENCE)))
	{
		cout << "Could not get player base address pointer. . . " << endl;
		cout << "Exiting . . ." << endl;
		delete bypass;
		exit(-1);
	}
	BASE_POINTER = BASE_DEREFERENCE;
	// Enemy one base address.
	if(!bypass->Read(BASE_ADDRESS_ENEMY1,&BASE_DEREFERENCE_EM1,sizeof(BASE_DEREFERENCE_EM1)))
	{
		cout << "Could not get enemy one base address pointer. . . " << endl;
		cout << "Exiting . . ." << endl;
	  delete bypass;
		exit(-1);
	}
	// Second dereference.
	BASE_POINTER_ENEMY1 = BASE_DEREFERENCE_EM1;
	if(!bypass->Read(BASE_POINTER_ENEMY1,&BASE_DEREFERENCE_EM1,sizeof(BASE_DEREFERENCE_EM1)))
	{
		cout << "Could not get enemy one base address pointer offset. . . " << endl;
		cout << "Exiting . . ." << endl;
		delete bypass;
		exit(-1);
	}
	BASE_POINTER_ENEMY1 = BASE_DEREFERENCE_EM1;

	cout << "Player and Enemy1 base address found. Press any key to start" << endl;
	cin.ignore();
	bool doExit = false;
	bool aimBotEnabled = false;
	bool godMode = false;
	int intToWrite = 1337;
	while(!doExit)
		{
			// Get pointer offset values, store into player.
			if(!(ObtainPlayerData(bypass,BASE_POINTER,player,&current)))
			{
				cout << "Could not get all player memory. . . ";
				cout << "Exiting . . ." << endl;
				delete bypass;
				exit(-1);
			}
			if(!(ObtainPlayerData(bypass,BASE_POINTER_ENEMY1,player,&enemyOne)))
			{
				cout << "Could not get all enemy memory. . . ";
				cout << "Exiting . . ." << endl;
				delete bypass;
				exit(-1);
			}
			if(godMode)
			{
				//GIVE HEALTH + AMMO
				if(!bypass->Write((BASE_POINTER+player.assaultRifleAmmo),&intToWrite,sizeof(intToWrite)))
				{
					cout << "Could not write to player ammo memory. . .";
					cout << "Exiting . . ." << endl;
					delete bypass;
					exit(-1);
				}
				if(!bypass->Write((BASE_POINTER+player.playerHealth),&intToWrite,sizeof(intToWrite)))
				{
					cout << "Could not write to player health memory. . . ";
					cout << "Exiting . . ." << endl;
					delete bypass;
					exit(-1);
				}
			}
			system("clear");
			printf("Press F1 to Start Aim bot. \n");
			printf("Press F2 for god mode(1337 HP). \n");
			printf("Press F4 to stop program. \n");
			printf("DIST TO ENEMY: %.6f \n",Get3dDistance(current,enemyOne));
			PrintOutVariables(current,0);
			PrintOutVariables(enemyOne,1);
			fflush(stdout);
			if(GetAsyncKeyState(VK_F4))
			{
				doExit = true;
			}
			if(GetAsyncKeyState(VK_F2))
			{
				godMode = true;
			}
			if(GetAsyncKeyState(VK_F1))
			{
				aimBotEnabled = true;
			}

			// User shooting while aimbot enabled.
			if(((GetKeyState(VK_LBUTTON) & 0x100) != 0) && aimBotEnabled)
			{
				float newY = GetYaw(enemyOne,current);
				float newX = GetPitch(enemyOne,current);
				bool status;
				status = !bypass->Write((BASE_POINTER+player.playerXmouse),&newX,sizeof(newX));
				status = !bypass->Write((BASE_POINTER+player.playerYmouse),&newY,sizeof(newY));
				if(status==false)
				{
						cout<<"Failure to write to memory.."<<endl;
						exit(-1);
				}
			}
			if(!aimBotEnabled){Sleep(100);}
		}
	;
	cout << "Exiting . . ." << endl;
	exit(0);
}
