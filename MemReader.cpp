#include <iostream>
#include <string>
#include <windows.h>
#include <commctrl.h>
#include <stdint.h>
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
	UINT32 playerXPos = 0x34;
	UINT32 playerYPos = 0x36;
	UINT32 playerZPos = 0x38;
	UINT32 playerXmouse = 0x40; // Up down
	UINT32 playerYmouse = 0x44; // Rotation
	UINT32 assaultRifleAmmo = 0x150;
	UINT32 playerHealth = 0xF8;
	UINT32 playerArmor = 0xFC;
	UINT32 playerTeam = 0x32C;
};

struct PLAYER_VARS
{
	DWORD playerHealth;
	DWORD playerArmor;
	float playerXPos;
	float playerYPos;
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


// Takes a base address of player, and applies values with pointer offsets.
bool ObtainPlayerData(Bypass* hook, uintptr_t bPointer, AC_PLAYER_OFFSETS pOffset, PLAYER_VARS *player)
{
	bool RPM = true;
	RPM = hook->Read((bPointer+pOffset.playerHealth),&player->playerHealth,sizeof(player->playerHealth));
	RPM = hook->Read((bPointer+pOffset.playerArmor),&player->playerArmor,sizeof(player->playerArmor));
	RPM = hook->Read((bPointer+pOffset.playerXmouse),&player->playerXmouse,sizeof(player->playerXmouse));
	RPM = hook->Read((bPointer+pOffset.playerYmouse),&player->playerYmouse,sizeof(player->playerYmouse));
	//RPM = hook->Read((bPointer+pOffset.playerXPos),&player->playerXPos,sizeof(player->playerXPos));
	//RPM = hook->Read((bPointer+pOffset.playerYPos),&player->playerYPos,sizeof(player->playerYPos));
	//RPM = hook->Read((bPointer+pOffset.playerZPos),&player->playerZPos,sizeof(player->playerZPos));
	//RPM = hook->Read((bPointer+pOffset.playerTeam),&player->playerTeam,sizeof(player->playerTeam));
	return RPM;
}

void PrintOutVariables(PLAYER_VARS player)
{

	printf("CURRENT Health: %u \n", player.playerHealth);
	printf("CURRENT Armor: %u \n", player.playerArmor);
	printf("CURRENT Mouse X: %.6f  \n", player.playerXmouse);
	printf("CURRENT Mouse Y: %.6f \n",player.playerYmouse);
	printf("CURRENT X Position: %.6f \n",player.playerXPos);
	printf("CURRENT Y Position: %.6f \n",player.playerYPos);
	printf("CURRENT Z Position: %.6f \n",player.playerZPos);
	fflush(stdout);

}


int main()
{

	AC_PLAYER_OFFSETS player;
	PLAYER_VARS current;
	PLAYER_VARS enemyOne;
	// Static base adress 0x400000 + 109B74
	uintptr_t BASE_ADDRESS = 0x509B74;
	uintptr_t BASE_POINTER;

	uintptr_t BASE_ADDRESS_ENEMY1;
	uintptr_t BASE_ADDRESS_ENEMY2;
	uintptr_t BASE_POINTER_ENEMY1;
	uintptr_t BASE_POINTER_ENEMY2;

	DWORD pid=0;
	DWORD BASE_DEREFERENCE = 0x0;


	cout << "Searching for Gamewindow. . ."<< endl;
	HWND hwnd = FindWindow(NULL, "AssaultCube");
	if(hwnd == NULL)
	{
		cout << "Cannot locate window. . .\nExiting.";
		exit(-1);
	}
	cout << "Gamewindow found!" << endl << endl;
	cout << "Obtaining Thread PID ... "<< endl;;
	GetWindowThreadProcessId(hwnd,&pid);
	if(pid==0)
	{
		cout << "Cannot get PID. . . Exiting.";
		exit(-1);
	}
	cout <<"PID Found" << endl << endl;
	cout << "Attempting to open handle on process: " << pid << " . . . " << endl;
	Bypass* bypass = new Bypass();
	if (!bypass->Attach(pid))
	{
		cout << "OpenProcess failed. GetLastError = " << dec << GetLastError() << endl;
		exit(-1);
	}

	cout << "Process handle opened" << endl << endl;
	cout << "Attempting to get base address:" << hex << uppercase << BASE_ADDRESS<< " pointer value. . ." << endl;

	if(!bypass->Read(BASE_ADDRESS,&BASE_DEREFERENCE,sizeof(BASE_DEREFERENCE)))
	{
		cout << "Could not get base address pointer. . . " << endl;
		exit(-1);
	}

	BASE_POINTER = BASE_DEREFERENCE;
	cout << "Player Base Pointer: " << uppercase << hex << &BASE_POINTER << endl;
	cout << "Press any key to being reading player active memory";
	cin.ignore();
	bool doExit = false;
	while(!doExit)
		{
			// Get pointer offset values, store into player.
			if(!(ObtainPlayerData(bypass,BASE_POINTER,player,&current)))
			{
				cout << "Could not get all player memory. . . ";
				exit(-1);
			}
			//TODO Get Enemy data.
			system("clear");
			printf("Press EXIT to stop. \n");
			PrintOutVariables(current);
			if(GetAsyncKeyState(VK_F1))
			{
				doExit = true;
			}
			Sleep(100);
		}
	cout << "Exiting . . ." << endl;
	exit(0);
}
