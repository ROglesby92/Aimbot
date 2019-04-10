#include <iostream>
#include <Windows.h>
#include <string>

#define CHAR_ARRAY_SIZE 128

using namespace std;

struct AC_PLAYER {

	UINT32 playerView = 0x40;
	UINT32 cameraAngle = 0x44;
};


int main()
{
	DWORD pid = 0;
	cout << "PID of Process: ";
	cin >> dec >> pid;
	
	cout << "Attempting to open handle on process..." << endl;

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	
	if (hProcess == NULL) 
	{
		cout << "Could not open process. GetLastError = " << dec << GetLastError() << endl;
		system("pause");
		return EXIT_FAILURE;
	}
	

	cout << "Handle successfully opened" << endl;

}

