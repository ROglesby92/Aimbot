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
}offsets;

struct Addresses
{
	const DWORD numEntites = 0x50F500;
	const DWORD entityList = 0x50F4F8;
	const DWORD playerBase = 0x509B74;
	const DWORD enemyOneBase = 0x510D90;
}addresses;


struct PlayerDataVector
{
	int playerHealth;
	float playerXPos;
	float playerYPos;
	float playerHeadPos;
	float playerZPos;
	float playerXmouse;
	float playerYmouse;
	int playerTeam;
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
bool ObtainPlayerData(Bypass* hook, uintptr_t bPointer, PlayerDataVector *player);
void PrintOutVariables(PlayerDataVector player,int playerNum);
float GetPitch(PlayerDataVector enemy, PlayerDataVector player);
float GetYaw(PlayerDataVector enemy, PlayerDataVector player);


// Takes a base address of player, and applies values with pointer offsets.
bool ObtainPlayerData(Bypass* hook, uintptr_t bPointer, PlayerDataVector *player)
{
	bool RPM;
	RPM = hook->Read((bPointer+offsets.playerHealth),&player->playerHealth,sizeof(player->playerHealth));
	RPM = hook->Read((bPointer+offsets.playerXmouse),&player->playerXmouse,sizeof(player->playerXmouse));
	RPM = hook->Read((bPointer+offsets.playerYmouse),&player->playerYmouse,sizeof(player->playerYmouse));
	RPM = hook->Read((bPointer+offsets.playerXPos),&player->playerXPos,sizeof(player->playerXPos));
	RPM = hook->Read((bPointer+offsets.playerYPos),&player->playerYPos,sizeof(player->playerYPos));
	RPM = hook->Read((bPointer+offsets.playerZPos),&player->playerZPos,sizeof(player->playerZPos));
	RPM = hook->Read((bPointer+offsets.playerHeadPos),&player->playerHeadPos,sizeof(player->playerHeadPos));
	RPM = hook->Read((bPointer+offsets.playerTeam),&player->playerTeam,sizeof(player->playerTeam));
	return RPM;
}

void PrintOutVariables(PlayerDataVector player)
{

	printf("----------------------------------\n");
	printf("PLAYER Team: %d \n",player.playerTeam);
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

PlayerDataVector GetClosestEntity(Bypass* bypass, PlayerDataVector player)
{

	// Get number of entites.
	int numEntitesRead;
	if(!bypass->Read(addresses.numEntites,&numEntitesRead,sizeof(numEntitesRead)))
	{
		cout << "Could not get number of entites";delete bypass;exit(-1);
	}
	// Dereference entity list pointer.
	uintptr_t entityListPtr = addresses.entityList;
	DWORD ENTITY_DEREFERENCE;

	if(!bypass->Read(entityListPtr,&ENTITY_DEREFERENCE,sizeof(ENTITY_DEREFERENCE)))
	{
		cout << "Cannot get entity list pointer";delete bypass;exit(-1);
	}
	// ENTITY_DEREFERENCE now stores address of pointer to entity list.
	// adding Offset value will take us to our entites.
	DWORD currentOffset = 0x04;
	// Entity to return.
	PlayerDataVector closestEntityFound;
	float closestDistance = 9999;
	for(int i=0;i<numEntitesRead-1;i++)
	{
		// Entity to copy.
		PlayerDataVector _entityToCopy;
		DWORD TEMP_DEREFERENCE=ENTITY_DEREFERENCE;
		// Get dereference to current entity.
		uintptr_t currentEntity = TEMP_DEREFERENCE+currentOffset;
		if(!bypass->Read(currentEntity, &TEMP_DEREFERENCE, sizeof(TEMP_DEREFERENCE)))
		{
			cout << "Cannot get entity: "<< i ;delete bypass;exit(-1);
		}
		// Pointer to current entity.
		currentEntity = TEMP_DEREFERENCE;
		// Get Entity data.
		if(!ObtainPlayerData(bypass,currentEntity,&_entityToCopy))
		{
			cout << "Cannot get entity: "<< i << " playerdata";delete bypass;exit(-1);
		}
		//TODO check team first..
		int entityHP;
		if(!bypass->Read(currentEntity+offsets.playerHealth, &entityHP, sizeof(entityHP)))
		{
			cout << "Cannot get entity: "<< i <<" hp..";delete bypass;exit(-1);
		}
		// Make sure entity is alive and is NOT on our team.
		if(entityHP > 0 && _entityToCopy.playerTeam != player.playerTeam)
		{
			float distToEnt = Get3dDistance(player,_entityToCopy);
			if(distToEnt < closestDistance)
			{
				closestDistance = distToEnt;
				closestEntityFound = _entityToCopy;
			}
		}
		// Increase offset for next entity.
		currentOffset+= 0x04;
	 }
	 return closestEntityFound;

}

int main()
{
	bool doExit = false;
	bool aimBotEnabled = false;
	bool godMode = false;
	int godModeInt = 1337;
	int numEntitesRead;

	PlayerDataVector currentPlayer;
	PlayerDataVector closestEntity;
	// Static base adress 0x400000 + 109B74
	uintptr_t BASE_ADDRESS = addresses.playerBase;
	DWORD pid=0;
	DWORD BASE_DEREFERENCE = 0x0;

	cout << "Searching for Gamewindow. . ."<< endl;
	HWND hwnd = FindWindow(NULL, "AssaultCube");
	if(hwnd == NULL){cout << "Cannot locate window. . .\nExiting.";exit(-1);}
	cout << "Obtaining Thread PID ... "<< endl;

	GetWindowThreadProcessId(hwnd,&pid);
	if(pid==0){cout << "Cannot get PID. . . Exiting.";exit(-1);}
	cout << "Attempting to open handle on process: "<<pid<<" ..."<<endl;

	Bypass* bypass = new Bypass();
	if (!bypass->Attach(pid)){
		cout << "OpenProcess failed. GetLastError = " << dec << GetLastError() << endl;delete bypass;exit(-1);
	}
	if(!bypass->Read(BASE_ADDRESS,&BASE_DEREFERENCE,sizeof(BASE_DEREFERENCE))){
		cout << "Could not get player base address pointer. . . " << endl;delete bypass;exit(-1);
	}
	BASE_ADDRESS = BASE_DEREFERENCE;

	// Aimbot loop
	while(!doExit)
		{
			if(!bypass->Read(addresses.numEntites,&numEntitesRead,sizeof(numEntitesRead))){
				cout << "Could not read number of entites..";delete bypass;exit(-1);
			}

			// Get player values
			if(!(ObtainPlayerData(bypass,BASE_ADDRESS,&currentPlayer)))
			{
				cout << "Could not read all player memory. . . ";delete bypass;exit(-1);
			}
			// Get closest enitity.
			closestEntity = GetClosestEntity(bypass,currentPlayer);
			//closestEntity = targetEnt;
			if(godMode)
			{
				//GIVE HEALTH + AMMO
				if(!bypass->Write((BASE_ADDRESS+offsets.assaultRifleAmmo),&godModeInt,sizeof(godModeInt)))
				{
					cout << "Could not write to player ammo memory. . .";
					cout << "Exiting . . ." << endl;
					delete bypass;
					exit(-1);
				}
				if(!bypass->Write((BASE_ADDRESS+offsets.playerHealth),&godModeInt,sizeof(godModeInt)))
				{
					cout << "Could not write to player health memory. . . ";
					cout << "Exiting . . ." << endl;
					delete bypass;
					exit(-1);
				}
			}
			system("clear");
			printf("Tap F1 to toggle Aim bot \n");
			printf("Tap F2 to toggle Infinite HP+Ammo \n");
			printf("Tap F3 to toggle ESP(Still in progess...) \n");
			printf("Press F4 to  Quit Program. \n");
			printf("%s \n", aimBotEnabled ? "Aimbot: Enabled" : "Aimbot: Disabled");
			printf("%s \n", godMode ? "Godmode: Enabled" : "Godmode: Disabled");
			printf("NUMBER OF ENTITES: %d \n",numEntitesRead);
			printf("DIST TO CLOSEST ENEMY: %.6f \n",Get3dDistance(currentPlayer,closestEntity));
			PrintOutVariables(closestEntity);
			fflush(stdout);
			if(GetAsyncKeyState(VK_F4))
			{
				doExit = true;
			}
			if(GetAsyncKeyState(VK_F2))
			{
				godMode = !godMode;
			}
			if(GetAsyncKeyState(VK_F1))
			{
				aimBotEnabled = !aimBotEnabled;
			}

			// User shooting while aimbot enabled.
			if(((GetKeyState(VK_LBUTTON) & 0x100) != 0) && aimBotEnabled)
			{
				float newY = GetYaw(closestEntity,currentPlayer);
				float newX = GetPitch(closestEntity,currentPlayer);
				bool status;
				status = !bypass->Write((BASE_ADDRESS+offsets.playerXmouse),&newX,sizeof(newX));
				status = !bypass->Write((BASE_ADDRESS+offsets.playerYmouse),&newY,sizeof(newY));
			}
			if(!aimBotEnabled){Sleep(50);}
		}
	;
	cout << "Exiting . . ." << endl;
	exit(0);
}
