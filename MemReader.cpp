#include <iostream>
#include <string>
#include <windows.h>
#include <commctrl.h>
#include <stdint.h>
#include "Math.h"
#include <sstream>
using namespace std;

HDC HDC_Desktop;
HBRUSH EnemyBrush;
HBRUSH FriendlyBrush;
HBRUSH currentBrush;
HFONT Font;
RECT m_Rect;
HWND TargetWnd;
HWND drawHandle;
DWORD DwProcId;
COLORREF HPCOLOR;
COLORREF NameCOLOR;




// Found offsets from ASSAULT_CUBE base address.
struct AC_PLAYER_OFFSETS
{
	UINT32 playerXPos = 0x04;
	UINT32 playerYPos = 0x08;
	UINT32 playerZPos = 0x0C;
	UINT32 playerZBodyPos = 0x3C;
	UINT32 playerXmouse = 0x44;
	UINT32 playerYmouse = 0x40;
	UINT32 playerHealth = 0xF8;
	UINT32 playerArmor = 0xFC;
	UINT32 playerTeam = 0x32C;
	UINT32 assaultRifleAmmo = 0x150;
	UINT32 playerName = 0x225;
}offsets;

struct Addresses
{
	const DWORD numEntites = 0x50F500;
	const DWORD entityList = 0x50F4F8;
	const DWORD playerBase = 0x509B74;
	const DWORD enemyOneBase = 0x510D90;
	const DWORD viewMatrix = 0x501AE8;
}addresses;


struct PlayerDataVector
{
	int playerHealth;
	float playerXPos;
	float playerYPos;
	float playerZBody;
	float playerZPos;
	float playerXmouse;
	float playerYmouse;
	int playerTeam;
	char playerName[24];
};

class Vec3 {
public:
	float x, y, z;
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
bool WorldToScreen(Vec3 In, Vec3& Out, float *viewMatrix);
void DrawString(int x, int y, COLORREF color, const char* text);
void DrawFilledRect(int x, int y, int w, int h);
void DrawBorderBox(int x, int y, int w, int h, int thickness);
void SetupDrawing(HDC hDesktop, HWND handle);


void SetupDrawing(HDC hDesktop, HWND handle)
{
	HDC_Desktop = hDesktop;
	drawHandle = handle;
	EnemyBrush = CreateSolidBrush(RGB(255, 0, 0));
	FriendlyBrush = CreateSolidBrush(RGB(0, 255, 0));
	NameCOLOR = RGB(255, 255, 0);
	HPCOLOR = RGB(0, 255, 0);
}

bool WorldToScreen(Vec3 In, Vec3& Out, float *ViewMatrix) {

	Out.x = In.x * ViewMatrix[0] + In.y * ViewMatrix[4] + In.z * ViewMatrix[8] + ViewMatrix[12];
	Out.y = In.x * ViewMatrix[1] + In.y * ViewMatrix[5] + In.z * ViewMatrix[9] + ViewMatrix[13];
	Out.z = In.x * ViewMatrix[2] + In.y * ViewMatrix[6] + In.z * ViewMatrix[10] + ViewMatrix[14];
	
	float w = In.x * ViewMatrix[3] + In.y * ViewMatrix[7] + In.z * ViewMatrix[11] + ViewMatrix[15];

	if (w < 0)
		return false;

	Out.x /= w;
	Out.y /= w;
	Out.z /= w;

	Out.x *= 800 / 2.0f;
	Out.x += 800 / 2.0f;

	Out.y *= -600 / 2.0f;
	Out.y += 600 / 2.0f;

	return true;
}

// Takes a base address of player, and applies values with pointer offsets.
bool ObtainPlayerData(Bypass* hook, uintptr_t bPointer, PlayerDataVector *player)
{
	bool RPM;
	RPM = hook->Read((bPointer+offsets.playerHealth),&player->playerHealth,sizeof(player->playerHealth));
	RPM = hook->Read((bPointer+offsets.playerXmouse),&player->playerXmouse,sizeof(player->playerXmouse));
	RPM = hook->Read((bPointer+offsets.playerYmouse),&player->playerYmouse,sizeof(player->playerYmouse));
	RPM = hook->Read((bPointer+offsets.playerXPos),&player->playerXPos,sizeof(player->playerXPos));
	RPM = hook->Read((bPointer+offsets.playerZBodyPos),&player->playerZBody,sizeof(player->playerZBody));
	RPM = hook->Read((bPointer+offsets.playerZPos),&player->playerZPos,sizeof(player->playerZPos));
	RPM = hook->Read((bPointer+offsets.playerYPos),&player->playerYPos,sizeof(player->playerYPos));
	RPM = hook->Read((bPointer+offsets.playerTeam),&player->playerTeam,sizeof(player->playerTeam));
	RPM = hook->Read((bPointer + offsets.playerName), &player->playerName, sizeof(player->playerName));
	return RPM;
}


void PrintOutVariables(PlayerDataVector player)
{

	printf("----------------------------------\n");
	printf("PLAYER Name: %d \n",player.playerName);
	printf("Health: %u \n", player.playerHealth);
	printf("Mouse X: %.6f  \n", player.playerXmouse);
	printf("Mouse Y: %.6f \n",player.playerYmouse);
	printf("X Position: %.6f \n",player.playerXPos);
	printf("Y Position: %.6f \n",player.playerYPos);
	printf("Z Position: %.6f \n",player.playerZBody);
	printf("Head Position: %.6f \n",player.playerZPos);
	printf("----------------------------------\n");

}


float GetPitch(PlayerDataVector enemy, PlayerDataVector player)
{
	float pitchX = (float)atan2(enemy.playerZPos - player.playerZPos, Get3dDistance(enemy,player)) * 180 / 3.1459265f;
	return pitchX;

}
float GetYaw(PlayerDataVector enemy, PlayerDataVector player)
{
	float yawY = -(float)atan2(enemy.playerXPos - player.playerXPos, enemy.playerYPos - player.playerYPos ) / 3.1459265f * 180 + 180;
	return yawY;

}

float Get3dDistance(PlayerDataVector source, PlayerDataVector dest)
{
	return (float)(sqrt
		(
	pow((source.playerXPos - dest.playerXPos),2) +
	pow((source.playerZPos - dest.playerZPos),2) +
	pow((source.playerYPos - dest.playerYPos),2)
));
}

void DrawBorderBox(int x, int y, int w, int h, int thickness) {
	DrawFilledRect(x, y, w, thickness);
	DrawFilledRect(x, y, thickness, h);
	DrawFilledRect((x+w), y, thickness, h);
	DrawFilledRect(x, y+h, w+thickness, thickness);
}

void DrawFilledRect(int x, int y, int w, int h) {
	RECT rect = { x , y , x + w, y + h };
	FillRect(HDC_Desktop, &rect, currentBrush);
}

void DrawString(int x, int y, COLORREF color, const char* text)
{
	SetTextAlign(HDC_Desktop, TA_CENTER | TA_NOUPDATECP);
	SetBkColor(HDC_Desktop, RGB(0, 0, 0));
	SetBkMode(HDC_Desktop, TRANSPARENT);
	SetTextColor(HDC_Desktop, color);
	SelectObject(HDC_Desktop, Font);
	TextOutA(HDC_Desktop, x, y, text, strlen(text));
	DeleteObject(Font);
}

void DrawESP(int x, int y, float distance , PlayerDataVector target) 
{
	// Rectangle.
	int width = 800 / distance;
	int height = 600 / distance;
	DrawBorderBox(x - (width/2), y, width, height*3.3, 1.8);
	
	//DrawLine((m_Rect.right - m_Rect.left) / 2, m_Rect.bottom - m_Rect.top, x, y, SnapLineCOLOR);

	
	stringstream hp;
	//Health 
	hp<<"HP : " << target.playerHealth;
	char *hpInfo = new char[hp.str().size() + 1];
	strcpy(hpInfo, hp.str().c_str());
	DrawString(x, (y+height*2), NameCOLOR, hpInfo);
	delete[] hpInfo;
}


void ESP(Bypass* bypass, int numEnt, PlayerDataVector player) {
	
	GetWindowRect(FindWindow(NULL, "AssaultCube"), &m_Rect);
	uintptr_t entityListPtr = addresses.entityList;
	DWORD ENTITY_DEREFERENCE;
	if (!bypass->Read(entityListPtr, &ENTITY_DEREFERENCE, sizeof(ENTITY_DEREFERENCE)))
	{
		cout << "Cannot get entity list pointer in ESP"; delete bypass; exit(-1);
	}
	DWORD currentOffset = 0x04;
	for (int i = 0; i < numEnt - 1; i++)
	{
		DWORD TEMP_DEREFERENCE = ENTITY_DEREFERENCE;
		PlayerDataVector ESP_DataVector;
		// Get dereference to current entity.
		uintptr_t currentEntity = TEMP_DEREFERENCE + currentOffset;
		if (!bypass->Read(currentEntity, &TEMP_DEREFERENCE, sizeof(TEMP_DEREFERENCE)))
		{cout << "Cannot get entity: " << i; delete bypass; exit(-1);}
		// Pointer to current entity.
		currentEntity = TEMP_DEREFERENCE;
		// Get Entity data.
		if (!ObtainPlayerData(bypass, currentEntity, &ESP_DataVector))
		{cout << "Cannot get entity: " << i << " playerdata"; delete bypass; exit(-1);}
		// Check if dead
		if (ESP_DataVector.playerHealth < 0) 
		{
			currentOffset += 0x04;
			continue;
		}
		// Change color of box.
		if (ESP_DataVector.playerTeam == player.playerTeam)
		{
			currentBrush = FriendlyBrush;
		}
		else
		{
			currentBrush = EnemyBrush;
		}
		// Get cordinates.
		Vec3 enemyXY;
		Vec3 entityVEC;
		entityVEC.x = ESP_DataVector.playerXPos;
		entityVEC.y = ESP_DataVector.playerYPos;
		entityVEC.z = ESP_DataVector.playerZPos;
		
		float viewMatrix[16];
		// Get World Screen Matrix. make sure its correctly reading it. 
		if (!bypass->Read(addresses.viewMatrix, &viewMatrix, sizeof(viewMatrix))) {
			cout << "Cannot get View Matrix"; delete bypass; exit(-1);
		}
		
		bool onScreen = WorldToScreen(entityVEC, enemyXY, viewMatrix);

		if (onScreen) {
			// On screen. DRAW distance from enemy...
			DrawESP(enemyXY.x, enemyXY.y, Get3dDistance(player, ESP_DataVector), ESP_DataVector);
		}
		currentOffset += 0x04;
	}
	
}



// Set values of key presses (prevents spam)
void CheckKeyStatus(int &f1, int &f2, int &f3) 
{
	if (!(GetKeyState(112) & 0x8000))
	{
		f1 = 0;
	}

	if (!(GetKeyState(113) & 0x8000))
	{
		f2 = 0;
	}

	if (!(GetKeyState(114) & 0x8000))
	{
		f3 = 0;
	}
}

void PostStatusToGUI(bool aimBot, bool Godmode, bool esp)
{
	stringstream aimbotStatus,godmodeStatus,espStatus,instructions;
	//Health 
	if (aimBot==true) {aimbotStatus << "Aimbot:  ON";}
	else {aimbotStatus << "Aimbot: OFF";}
	if (Godmode == true) { godmodeStatus << "Godmode: ON";}
	else { godmodeStatus << "Godmode: OFF";}
	if (esp == true) {espStatus << "ESP:     ON";}
	else {espStatus << "ESP:     OFF";}
	instructions << "F1: Aimbot | F2: Godmode | F3: ESP | F4: Quit ";

	char *INST = new char[instructions.str().size() + 1]; strcpy(INST, instructions.str().c_str());
	char *AB = new char[aimbotStatus.str().size() + 1];strcpy(AB, aimbotStatus.str().c_str());
	char *GM = new char[godmodeStatus.str().size() + 1];strcpy(GM, godmodeStatus.str().c_str());
	char *ESP = new char[espStatus.str().size() + 1];strcpy(ESP, espStatus.str().c_str());
	
	DrawString(550, 5, HPCOLOR, INST);
	DrawString(455, 25, HPCOLOR, AB);
	DrawString(455, 45, HPCOLOR, GM);
	DrawString(455, 65, HPCOLOR, ESP);
	
	delete[] AB,GM,ESP,INST;

}

PlayerDataVector GetClosestEntity(Bypass* bypass, PlayerDataVector player, int numEnt)
{


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
	for(int i=0;i< numEnt -1;i++)
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
		// Make sure entity is alive and is NOT on our team AND we are looking at their direction.
		if(_entityToCopy.playerHealth > 0 && _entityToCopy.playerTeam != player.playerTeam)
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
	bool espOn = false;
	int godModeInt = 1337;
	int numEntitesRead;
	int godModePressed = 0;
	int espKeyPressed = 0;
	int aimbotPressed = 0;


	PlayerDataVector currentPlayer;
	PlayerDataVector closestEntity;
	// Static base adress 0x400000 + 109B74
	uintptr_t BASE_ADDRESS = addresses.playerBase;
	DWORD pid=0;
	DWORD BASE_DEREFERENCE = 0x0;

	cout << "Searching for Gamewindow..."<< endl;
	HWND hwnd = FindWindow(NULL, "AssaultCube");
	if(hwnd == NULL){cout << "Cannot locate window. . .\nExiting.";exit(-1);}
	// Connect window handle for ESP
	
	TargetWnd = hwnd;
	HDC HDC_Desktop = GetDC(TargetWnd);
	SetupDrawing(HDC_Desktop, TargetWnd);
	
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
	cout << "Cheats enabled ... F4 to EXIT" << endl;

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
			closestEntity = GetClosestEntity(bypass,currentPlayer,numEntitesRead);
			//closestEntity = targetEnt;
			if (espOn) {ESP(bypass,numEntitesRead,currentPlayer);}
			
			if(godMode)
			{
				//GIVE HEALTH + AMMO
				if(!bypass->Write((BASE_ADDRESS+offsets.assaultRifleAmmo),&godModeInt,sizeof(godModeInt)))
				{
					cout << "Could not write to player ammo memory. . .";delete bypass;exit(-1);
				}
				if(!bypass->Write((BASE_ADDRESS+offsets.playerHealth),&godModeInt,sizeof(godModeInt)))
				{
					cout << "Could not write to player health memory. . . ";delete bypass;exit(-1);
				}
			}
			
		
			
			
			if(GetAsyncKeyState(VK_F4))
			{
				doExit = true;
			}
			if(GetAsyncKeyState(VK_F2) && godModePressed==0)
			{
				godModePressed = 1;
				godMode = !godMode;
			}
			if (GetAsyncKeyState(VK_F3) && espKeyPressed == 0)
			{
				espKeyPressed = 1;
				espOn = !espOn;
			}
			if(GetAsyncKeyState(VK_F1) && aimbotPressed == 0)
			{
				aimbotPressed = 1;
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
			CheckKeyStatus(aimbotPressed, godModePressed, espKeyPressed);
			PostStatusToGUI(aimBotEnabled, godMode, espOn);
			
			
			
		}
	;
	cout << "Exiting ..." << endl;
	exit(0);
}
