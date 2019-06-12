#include <iostream>
#include <string>
#include <windows.h>
#include <commctrl.h>
#include <stdint.h>
#include <sstream>
#include <TlHelp32.h>
#include "csgo.hpp"
#include "Math.h"
#include <tchar.h>

using namespace std;
using namespace hazedumper::netvars;
using namespace hazedumper::signatures;

int _SCREENWIDTH=852;
int _SCREENHEIGHT=480;

struct GAME_VARIABLES {
	DWORD game_Module;
	DWORD dwEngineBase;
	DWORD localPlayer;
}G_VARS;

class Vec3 {
public:
	float x, y, z;
};

struct LOCAL_PLAYER_VARS {
	int playerHealth;
	int playerArmor;
	float playerXPos;
	float playerYPos;
	float playerZBody;
	float playerZPos;
	int playerTeam;
	Vec3 headPosition;
	Vec3 entityLocation;
}LOCAL_PLAYER;

struct PLAYER_ENTITY_VARS 
{
	int playerHealth;
	int playerArmor;
	float playerXPos;
	float playerYPos;
	float playerZBody;
	float playerZPos;
	float playerXmouse;
	float playerYmouse;
	int playerTeam;
	bool isDormant;
	bool isVisible;
	Vec3 headPosition;
	Vec3 entityLocation;
};

// Function references.



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

// Declare function references.
PLAYER_ENTITY_VARS FindClosestEnemyEntity(Bypass *bypass);
float Get3dDistance(PLAYER_ENTITY_VARS source);
float NormalizeAngle(float angle, float upper, float lower);
float ClampAngle(float angle, float upper, float lower, float clamp);
bool IsVisible(Bypass* bypass, DWORD local, DWORD entity);
bool GetLocalPlayerData(Bypass *bypass);
bool AutoAim(Bypass* bypass);
void ExtraSensoryPerception(Bypass *bypass);

// Checks if target is in sight of our player
bool IsVisible(Bypass* bypass, DWORD local, DWORD entity){
	bool status;
	int mask, playerBase;
	
	status = bypass->Read(entity + hazedumper::netvars::m_bSpottedByMask, &mask, sizeof(mask));
	status = bypass->Read(local + 0x64, &playerBase, sizeof(playerBase));

	playerBase -= 1;
    return (mask & (1 << playerBase)) > 0;
}

float Get3dDistance(PLAYER_ENTITY_VARS dest)
{
	return (float)(sqrt
	(
		pow((LOCAL_PLAYER.playerXPos - dest.playerXPos), 2) +
		pow((LOCAL_PLAYER.playerZPos - dest.playerZPos), 2) +
		pow((LOCAL_PLAYER.playerYPos - dest.playerYPos), 2)
	));
}

void CalculateAngle(Vec3 source, Vec3 dest, Vec3 &Angles) 
{
	double DELTA[3] = { (source.x - dest.x),(source.y - dest.y),(source.z - dest.z) };
	double HYP = sqrt(DELTA[0] * DELTA[0] + DELTA[1] * DELTA[1]);
	Angles.x = (float)(asinf(DELTA[2] / HYP)*57.295779513082f);
	Angles.y = (float)(atanf(DELTA[1] / DELTA[0])*57.295779513082f);
	Angles.z = 0.0f;
	if (DELTA[0] >= 0.0) { Angles.y += 180.0f; }
}

bool GetLocalPlayerData(Bypass *bypass) 
{
	
	//Get Head
	bool status;

	status = bypass->Read(G_VARS.localPlayer + hazedumper::netvars::m_iTeamNum, &LOCAL_PLAYER.playerTeam, sizeof(LOCAL_PLAYER.playerTeam));
	status = bypass->Read(G_VARS.localPlayer + hazedumper::netvars::m_iHealth, &LOCAL_PLAYER.playerHealth, sizeof(LOCAL_PLAYER.playerHealth));
	status = bypass->Read(G_VARS.localPlayer + hazedumper::netvars::m_ArmorValue, &LOCAL_PLAYER.playerArmor, sizeof(LOCAL_PLAYER.playerArmor));
	status = bypass->Read(G_VARS.localPlayer + hazedumper::netvars::m_vecOrigin + 0x0, &LOCAL_PLAYER.playerXPos, sizeof(LOCAL_PLAYER.playerXPos));
	status = bypass->Read(G_VARS.localPlayer + hazedumper::netvars::m_vecOrigin + 0x4, &LOCAL_PLAYER.playerYPos, sizeof(LOCAL_PLAYER.playerYPos));
	status = bypass->Read(G_VARS.localPlayer + hazedumper::netvars::m_vecOrigin + 0x8, &LOCAL_PLAYER.playerZPos, sizeof(LOCAL_PLAYER.playerZPos));
	
	status = bypass->Read(G_VARS.localPlayer + hazedumper::netvars::m_vecOrigin + 0x0, &LOCAL_PLAYER.entityLocation.x, sizeof(LOCAL_PLAYER.entityLocation.x));
	status = bypass->Read(G_VARS.localPlayer + hazedumper::netvars::m_vecOrigin + 0x4, &LOCAL_PLAYER.entityLocation.y, sizeof(LOCAL_PLAYER.entityLocation.y));
	status = bypass->Read(G_VARS.localPlayer + hazedumper::netvars::m_vecOrigin + 0x8, &LOCAL_PLAYER.entityLocation.z, sizeof(LOCAL_PLAYER.entityLocation.z));


	return status;
}

bool GetEntityVariables(Bypass *bypass, DWORD entityAddress,PLAYER_ENTITY_VARS *vars) 
{
	bool status;
	status = bypass->Read(entityAddress + hazedumper::netvars::m_iTeamNum, &vars->playerTeam, sizeof(vars->playerTeam));
	status = bypass->Read(entityAddress + hazedumper::netvars::m_iHealth, &vars->playerHealth, sizeof(vars->playerHealth));
	status = bypass->Read(entityAddress + hazedumper::netvars::m_ArmorValue, &vars->playerArmor, sizeof(vars->playerArmor));
	status = bypass->Read(entityAddress + hazedumper::netvars::m_vecOrigin + 0x0, &vars->playerXPos, sizeof(vars->playerXPos));
	status = bypass->Read(entityAddress + hazedumper::netvars::m_vecOrigin + 0x4, &vars->playerYPos, sizeof(vars->playerYPos));
	status = bypass->Read(entityAddress + hazedumper::netvars::m_vecOrigin + 0x8, &vars->playerZPos, sizeof(vars->playerZPos));
	status = bypass->Read(entityAddress + hazedumper::signatures::m_bDormant, &vars->isDormant, sizeof(vars->isDormant));
	vars->isVisible = IsVisible(bypass, G_VARS.localPlayer, entityAddress);
	status = bypass->Read(entityAddress + hazedumper::netvars::m_vecOrigin + 0x0, &vars->entityLocation.x, sizeof(vars->entityLocation.x));
	status = bypass->Read(entityAddress + hazedumper::netvars::m_vecOrigin + 0x4, &vars->entityLocation.y, sizeof(vars->entityLocation.y));
	status = bypass->Read(entityAddress + hazedumper::netvars::m_vecOrigin + 0x8, &vars->entityLocation.z, sizeof(vars->entityLocation.z));

	return status;
}

uintptr_t GetModuleBaseAddress(DWORD procId, const TCHAR* modName)
{
	uintptr_t modBaseAddr = 0;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 modEntry;
		modEntry.dwSize = sizeof(modEntry);
		if (Module32First(hSnap, &modEntry))
		{
			do
			{
				if (!lstrcmpi(modEntry.szModule, modName))
				{
					modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
					break;
				}
			} while (Module32Next(hSnap, &modEntry));
		}
	}
	CloseHandle(hSnap);
	return modBaseAddr;
}

PLAYER_ENTITY_VARS FindClosestEnemyEntity(Bypass *bypass) 
{
	uintptr_t entityListptr = G_VARS.game_Module + hazedumper::signatures::dwEntityList;
	PLAYER_ENTITY_VARS entityToReturn;
	entityToReturn.playerXPos = 0; entityToReturn.playerYPos = 0; entityToReturn.playerZPos = 0; entityToReturn.isDormant = true;
	float closestDist = 99999;
	for (int i = 0; i < 64; i++)
	{
		PLAYER_ENTITY_VARS eVars;
		DWORD entity;
		if (bypass->Read(entityListptr + (i * 0x10), &entity, sizeof(entity))) 
		{
			if (entity == NULL) { continue; }
			if (!GetEntityVariables(bypass, entity, &eVars))
			{cout << "Cannot retrieve entity memory: " << i << " , ENTITY =  " << entity; exit(-1);}
			// Is the entity: dead, dormant , friendly or offscreen? if so, continue.
			
			if (eVars.playerHealth <= 0 || eVars.isDormant || eVars.playerTeam == LOCAL_PLAYER.playerTeam || !eVars.isVisible) { continue; }
			else 
			{
				float dist = Get3dDistance(eVars);
				if (dist < closestDist) {
					closestDist = dist;
					entityToReturn = eVars;
				}
			}
		}
	}
	return entityToReturn;
}

// ESP -> Adds glow to non-dormant enemies and friendlys
void ExtraSensoryPerception(Bypass* bypass) {
	uintptr_t entityListptr = G_VARS.game_Module + hazedumper::signatures::dwEntityList;
	DWORD gObject;
	if (!bypass->Read(G_VARS.game_Module + hazedumper::signatures::dwGlowObjectManager, &gObject,sizeof(gObject))) 
	{cout << "Cannot read glowobject..." << endl; exit(-1);}
	float _zero = 0; float _two = 2; float _oneSeven = 1.7;
	bool rpm; bool _t = true; bool _f = false;
	for (int i = 0; i < 64; i++) 
	{
		PLAYER_ENTITY_VARS eVars;
		DWORD entity;
		if (bypass->Read(entityListptr + (i * 0x10), &entity, sizeof(entity)))
		{
			
			if (entity == NULL) { continue; }
			int glowIdx;
			rpm = bypass->Read(entity + hazedumper::netvars::m_iGlowIndex, &glowIdx, sizeof(glowIdx));
			if (!GetEntityVariables(bypass, entity, &eVars))
			{cout << "Cannot retrieve entity memory: " << i; exit(-1);}
			//Make sure they are not dead.  Check if dormant?
			if (eVars.playerHealth <= 0 || eVars.playerHealth > 100 || eVars.isDormant) { continue; }
			// Check team
			if (eVars.playerTeam == LOCAL_PLAYER.playerTeam) 
			{
				rpm = bypass->Write(gObject + ((glowIdx * 0x38) + 0x4), &_two, sizeof(_two));
				rpm = bypass->Write(gObject + ((glowIdx * 0x38) + 0x8), &_two, sizeof(_zero));
				rpm = bypass->Write(gObject + ((glowIdx * 0x38) + 0xC), &_zero, sizeof(_zero));
				rpm = bypass->Write(gObject + ((glowIdx * 0x38) + 0x10), &_oneSeven, sizeof(_oneSeven));
			}
			else
			{
				rpm = bypass->Write(gObject + ((glowIdx * 0x38) + 0x4), &_zero, sizeof(_zero));
				rpm = bypass->Write(gObject + ((glowIdx * 0x38) + 0x8), &_oneSeven, sizeof(_oneSeven));
				rpm = bypass->Write(gObject + ((glowIdx * 0x38) + 0xC), &_oneSeven, sizeof(_oneSeven));
				rpm = bypass->Write(gObject + ((glowIdx * 0x38) + 0x10), &_oneSeven, sizeof(_oneSeven));
			}
			rpm = bypass->Write(gObject + ((glowIdx * 0x38) + 0x24), &_t, sizeof(_t));
			rpm = bypass->Write(gObject + ((glowIdx * 0x38) + 0x25), &_f, sizeof(_f));
		}
	}
}

// Determine closest enemy, then evaluate pitch/yaw for view angles and set them.
bool AutoAim(Bypass* bypass) 
{
	
	//Make sure player is not dead.
	if (LOCAL_PLAYER.playerHealth <= 0 || LOCAL_PLAYER.playerHealth > 100) { return true; }

	// Locate closest agent.
	PLAYER_ENTITY_VARS agent = FindClosestEnemyEntity(bypass);
	if (agent.isDormant || agent.playerHealth <= 0 || agent.playerHealth > 100 || !agent.isVisible) { return true; }

	// Get ClientState for view angles.
	DWORD clientSTATE;
	if (!bypass->Read(G_VARS.dwEngineBase + hazedumper::signatures::dwClientState, &clientSTATE, sizeof(clientSTATE))) {
		cout << "Cannot get client state" << endl; exit(-1);}
	
	
	Vec3 Angles;
	CalculateAngle(LOCAL_PLAYER.entityLocation, agent.entityLocation, Angles);
	
	float normalizedX = NormalizeAngle(Angles.x, 180.0f, -180.0f);
	float normalizedY = NormalizeAngle(Angles.y, 180.0f, -180.0f);
	float clampedX = ClampAngle(normalizedX, 89.0f, -89.0f, 89.0f);
	if (clampedX < 88.0f){clampedX += 1.25;}else{clampedX -= 1;}
	
	// SMOOTHING VARIABLES..
	float newYaw, newPitch;
	Vec3 Juice;
	float smooth = .04f;
	bool rpm;

	if (!bypass->Read(clientSTATE + hazedumper::signatures::dwClientState_ViewAngles, &Juice.x, sizeof(Juice.x))) {
		cout << "Cannot READ client state X" << endl; exit(-1);}
	if (!bypass->Read(clientSTATE + hazedumper::signatures::dwClientState_ViewAngles + 0x4, &Juice.y, sizeof(Juice.y))) {
		cout << "Cannot READ client state Y" << endl; exit(-1);}
	
	
	// DIFFERENCE FROM OUR VECTOR TO THEIRS.
	newYaw = normalizedY - Juice.y;
	newPitch = clampedX - Juice.x;
	// Lerp the distance
	for(int i = 0; i < 25; i++)
	{
		Juice.x += newPitch * smooth;
		Juice.y += newYaw * smooth;
		rpm = bypass->Write(clientSTATE + hazedumper::signatures::dwClientState_ViewAngles, &Juice.x, sizeof(Juice.x));
		rpm = bypass->Write(clientSTATE + hazedumper::signatures::dwClientState_ViewAngles + 0x4, &Juice.y, sizeof(Juice.y));
		Sleep(1);
	}

	return true;
	
}

//Normalize Angles before setting.
float NormalizeAngle(float angle, float upper, float lower) {
	while (lower > angle || angle > upper) {
		if (angle < lower) angle += 360.0f;
		if (angle > upper) angle -= 360.0f;
	}
	return angle;
}

//Clamp X
float ClampAngle(float angle, float upper, float lower, float clamp) {
	if (lower > angle || angle > upper) {
		return clamp;
	}
	return angle;
}

// Set values of key presses (prevents spam)
void CheckKeyStatus(int &f1, int &f2)
{
	if (!(GetKeyState(112) & 0x8000)) { f1 = 0; }
	if (!(GetKeyState(113) & 0x8000)) { f2 = 0; }
	//if (!(GetKeyState(114) & 0x8000)) { f3 = 0; }
}

void PrintStatusToTerminal(bool f1, bool f2)
{
	system("clear");
	cout << "F1: ESP  |  F2: Aimbot" << endl;
	cout << "ESP   : " << f1 << endl;
	cout << "AIMBOT: " << f2 << endl;

}

int main() {
	Bypass* bypass;
	DWORD PID=0;

	// Locate game window
	cout << "Searching for CSGO. . ." << endl;
	HWND hwnd = FindWindow(NULL, "Counter-Strike: Global Offensive");
	if (!hwnd) { cout << "Cannot locate CSGO"; return -1; }
	
	// Locate PID
	cout << "Obtaining Thread PID . . . " << endl;
	GetWindowThreadProcessId(hwnd, &PID);
	if (PID == 0) { cout << "Cannot get PID. . . Exiting."; exit(-1); }
	
	// Generate handle on process
	cout << "Attempting to open handle on process: " << PID << endl;
	
	if (!bypass->Attach(PID))
	{cout << "OpenProcess failed. "<< endl; exit(-1);}
	// Get Module Base Address for "client_panorama.dll" and "engine.dll"
	G_VARS.game_Module = (DWORD)GetModuleBaseAddress(PID, "client_panorama.dll");
	G_VARS.dwEngineBase = (DWORD)GetModuleBaseAddress(PID, "engine.dll");
	// Locate local player address.
	if (!bypass->Read(G_VARS.game_Module + hazedumper::signatures::dwLocalPlayer, &G_VARS.localPlayer, sizeof(G_VARS.localPlayer)))
	{cout << "Cannot find local player." << endl; exit(-1);}
	
	cout << " Ready to roll: F1: ESP | F2: AIMBOT " << endl;
	// Read memory in loop.
	bool _quit = false;
	bool _esp = false; int _f1 = 0;
	bool _aimbot = false; int _f2 = 0;

	while (1 && !_quit) 
	{
		if (!GetLocalPlayerData(bypass)) {cout << "Cannot retreive player memory";exit(-1);}
		
		if (_esp) {ExtraSensoryPerception(bypass);}
		
		if (_aimbot && ((GetKeyState(VK_RBUTTON) & 0x100) != 0)){
			if (!AutoAim(bypass))
			{cout << "Cannot write to memory for auto aim.." << endl; exit(-1);}
		}
		Sleep(1);
		if (GetAsyncKeyState(VK_F4)){_quit = true;}
		if (GetAsyncKeyState(VK_F1) && _f1 == 0) {
			_esp = !_esp; _f1 = 1; PrintStatusToTerminal(_esp,_aimbot);
		}
		
		if (GetAsyncKeyState(VK_F2) && _f2 == 0) 
		{
			_aimbot = !_aimbot; _f2 = 1; PrintStatusToTerminal(_esp, _aimbot);
		}
		CheckKeyStatus(_f1,_f2);
	}

	return 1;
}
