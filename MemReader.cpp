#include <iostream>
#include <string>
#include <windows.h>
#include <commctrl.h>
#include <stdint.h>

/*

First project attempting to read game memory to implement features such as:

AIMBOT
ESP
WRITING VALUES

*/


using namespace std;

// Found offsets from ASSAULT_CUBE base address.
struct AC_PLAYER 
{
	UINT32 playerView = 0x40;
	UINT32 cameraAngle = 0x44;
	UINT32 assaultRifleAmmo = 0x150;
	UINT32 pistolAmmo;
	UINT32 playerHealth;
	UINT32 playerArmor;
	
};

struct PLAYER_VALUES
{
	DWORD rifleAmmo;
	DWORD pistolAmmo;
	DWORD playerHealth;
	DWORD playerArmor;
};


int main()
{
	
	AC_PLAYER player;
	PLAYER_VALUES current;
	
	// Static base adress 
	uintptr_t BASE_ADDRESS = 0x509B74;
	// Pointer to our player
	uintptr_t BASE_POINTER;
	DWORD pid=0;
	DWORD BASE_DEREFERENCE = 0x0;
	DWORD playerAmmo = 0x150;
	DWORD intRead = 0x0;
	
	cout << "Searching for Gamewindow . . . " << endl;
	
	HWND hwnd = FindWindow(NULL, "AssaultCube");
	if(hwnd == NULL)
	{
		cout << "Cannot locate window.. Exiting.";
		return EXIT_FAILURE;
	}
	
	cout << "Getting Thread PID . . . " << endl;
	GetWindowThreadProcessId(hwnd,&pid);
	
	if(pid==0)
	{
		cout << "Cannot get PID.. Exiting.";
		return EXIT_FAILURE;
	}
	
	cout << "Attempting to open handle on process: " << pid << " . . . " << endl;
	HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);	
	
	if (pid == 0)
	{
		cout << "Cannot open process." << endl;
		Sleep(3000);
		exit(-1);
	}
	// Success
	cout << "Process handle opened" << endl;
	cout << "Attempting to get base address. . ." << endl;
	Sleep(250);
	
	if(!ReadProcessMemory(handle,(LPCVOID)BASE_ADDRESS,&BASE_DEREFERENCE,sizeof(BASE_DEREFERENCE),NULL))
	{
		cout << "Could not get base address pointer. . . " << endl;
	}
	
	BASE_POINTER = BASE_DEREFERENCE;
	cout << "Base address pointer reference located: " << uppercase << hex << &BASE_POINTER << endl;
	
	bool doExit = false;
	
	while(!doExit)
		{
			
			if(!ReadProcessMemory(handle,(LPCVOID)(BASE_POINTER+player.assaultRifleAmmo),&current.rifleAmmo,sizeof(current.rifleAmmo),NULL))
			{
				cout << "Memory unavaiable. . . " << endl;
				doExit = true;
			}
			else
			{
				//system("clear");
				//cout << "AMMO: " <<(signed long)intRead << endl;
				printf("CURRENT AMMO: %u \n", current.rifleAmmo);
				fflush(stdout);
				
			}
			
			if(GetAsyncKeyState(VK_ESCAPE))
			{
				doExit = true;
			}
			
			Sleep(500);
				
		}
	
	
	cout << "Exiting . . ." << endl;
	exit(0);


}
