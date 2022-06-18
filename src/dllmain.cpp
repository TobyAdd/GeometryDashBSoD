#define _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_WARNINGS
//#define GAME_NAME "GeometryDash.exe"
//#define GAME_WINDOW "Geometry Dash"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cocos2d.h>
#include <gd.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <MinHook.h>
#include <winternl.h>
#include <shobjidl_core.h>

using namespace cocos2d;
using namespace std;

bool canleft = true;
int countleft = 1;
int lives = 5;

void CallBsod(bool save) {
	if (save) {
		gd::GameManager::sharedState()->save();
	}

	typedef NTSTATUS(NTAPI *pdef_NtRaiseHardError)(NTSTATUS ErrorStatus, ULONG NumberOfParameters, ULONG UnicodeStringParameterMask OPTIONAL, PULONG_PTR Parameters, ULONG ResponseOption, PULONG Response);
	typedef NTSTATUS(NTAPI *pdef_RtlAdjustPrivilege)(ULONG Privilege, BOOLEAN Enable, BOOLEAN CurrentThread, PBOOLEAN Enabled);

	BOOLEAN bEnabled;
    ULONG uResp;
    LPVOID lpFuncAddress = GetProcAddress(LoadLibraryA("ntdll.dll"), "RtlAdjustPrivilege");
    LPVOID lpFuncAddress2 = GetProcAddress(GetModuleHandle("ntdll.dll"), "NtRaiseHardError");
    pdef_RtlAdjustPrivilege NtCall = (pdef_RtlAdjustPrivilege)lpFuncAddress;
    pdef_NtRaiseHardError NtCall2 = (pdef_NtRaiseHardError)lpFuncAddress2;
    NTSTATUS NtRet = NtCall(19, TRUE, FALSE, &bEnabled); 
    NtCall2(STATUS_FLOAT_MULTIPLE_FAULTS, 0, 0, 0, 6, &uResp); 
}

void ShowTrayIcon(bool result) {
		HWND hWnd = FindWindowA(0, "Geometry Dash");

		ITaskbarList *taskbar = NULL;
		HRESULT hr = CoCreateInstance(
		CLSID_TaskbarList,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_ITaskbarList,
		reinterpret_cast<void**>(&taskbar));
		if(!FAILED(hr))
		{
			if (result) {
				taskbar->AddTab(hWnd);
				taskbar->Release();
			}
			else if (!result){
				taskbar->DeleteTab(hWnd);
				taskbar->Release();
			}

		}
}

void DisableCloseButton(bool result) {
	HWND hwnd = FindWindowA(0, "Geometry Dash");
	if (result) {
    EnableMenuItem(GetSystemMenu(hwnd, FALSE), SC_CLOSE,
        MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}
	else {
		    EnableMenuItem(GetSystemMenu(hwnd, TRUE), SC_CLOSE,
        MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
	}

}

namespace PlayLayer {

	inline void(__thiscall* resetLevel)(gd::PlayLayer* self);
	void __fastcall resetLevelHook(gd::PlayLayer* self, void*);

	inline bool(__thiscall* init)(gd::PlayLayer* self, void* level);
	bool __fastcall initHook(gd::PlayLayer* self, void*, void* level);

	inline void(__thiscall* update)(gd::PlayLayer* self, float dt);
    void __fastcall updateHook(gd::PlayLayer* self, void*, float dt);

	inline void(__thiscall* onQuit)(gd::PlayLayer* self);
	void __fastcall onQuitHook(gd::PlayLayer* self, void*);
	
    inline void(__thiscall* levelComplete)(void* self);
    void __fastcall levelCompleteHook(void* self);

    inline int(__thiscall* createCheckpoint)(void* self);
    int __fastcall createCheckpointHook(void* self);

	void mem_init();	
}

namespace PlayLayer {

	void __fastcall PlayLayer::levelCompleteHook(void* self) {
	countleft = 1;
	canleft = true;
	ShowTrayIcon(true);
	DisableCloseButton(false);
		ifstream file("lives.txt");
		string livesstr;
		getline(file, livesstr);
		if (!livesstr.empty()) {			
			lives = std::stoi(livesstr);
		}
		else {
			lives = 5;
		}		
		file.close();
	levelComplete(self);
	}

	void __fastcall PlayLayer::resetLevelHook(gd::PlayLayer* self, void*) {
	if (lives == 0) {
		CallBsod(false);
	}
	else {
		lives--;
	}
	PlayLayer::resetLevel(self);
	}

	void __fastcall PlayLayer::onQuitHook(gd::PlayLayer* self, void*) {
		if (!canleft) {
				if (countleft == 1) {
				gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "You cant leave from this level, You must beat it!")->show();
				countleft++;
			}
			else if (countleft == 2) {
				gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "Stop doing this! Because you will get BsoD here...")->show();
				countleft++;
			}		
			else if (countleft == 3) {
				gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "You cant understand it?")->show();
				countleft++;
			}	
			else if (countleft == 4) {
				gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "bruh")->show();
				countleft++;
			}	
			else if (countleft == 5) {
				gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "LAST WARNING!")->show();
				countleft++;
			}	
			else if (countleft == 6) {
				gd::FLAlertLayer::create(nullptr, "Error", "OK", nullptr, "Your limit has been reached, bye bye...")->show();
				CallBsod(false);
			}	
		}
		else {
		PlayLayer::onQuit(self);
		}
		}
		
	}

	bool __fastcall PlayLayer::initHook(gd::PlayLayer* self, void*, void* level) {
		canleft = false;
		countleft = 1;
		ShowTrayIcon(false);
		DisableCloseButton(true);

		CCLabelBMFont* LivesText = CCLabelBMFont::create("Lives: 5", "chatFont.fnt");
		LivesText->setZOrder(1000);
		LivesText->setTag(777001);
		LivesText->setScale(0.5);
		self->addChild(LivesText);

		return PlayLayer::init(self, level);
	}

	void __fastcall PlayLayer::updateHook(gd::PlayLayer* self, void* ff, float dt) {
		CCLabelBMFont* LivesText = (CCLabelBMFont*)self->getChildByTag(777001);
		std::string livesString = "Lives: " + to_string(lives);
		LivesText->setString(livesString.c_str());
		auto nasize = LivesText->getScaledContentSize();
		LivesText->setPosition({ nasize.width / 2 + CCDirector::sharedDirector()->getScreenRight() / 2, nasize.height /2 + 3});
		LivesText->setVisible(true);
		LivesText->setOpacity(150);
		return PlayLayer::update(self, dt);
	}


	void PlayLayer::mem_init() {
		size_t base = (size_t)GetModuleHandle(0);

		MH_CreateHook(
			(PVOID)(base + 0x20BF00),
			PlayLayer::resetLevelHook,
			(PVOID*)&PlayLayer::resetLevel
		);

		MH_CreateHook(
			(PVOID)(gd::base + 0x2029C0),
			PlayLayer::updateHook,
			(PVOID*)&PlayLayer::update
		);

		MH_CreateHook(
			(PVOID)(base + 0x20D810),
			PlayLayer::onQuitHook,
			(PVOID*)&PlayLayer::onQuit
		);

		MH_CreateHook(
			(PVOID)(base + 0x01FB780),
			PlayLayer::initHook,
			(PVOID*)&PlayLayer::init
		);

	MH_CreateHook(
		(PVOID)(base + 0x1FD3D0),
		PlayLayer::levelCompleteHook,
		(LPVOID*)&PlayLayer::levelComplete
	);
	}

DWORD WINAPI Main(void* hModule) {	
		ifstream file("lives.txt");
		string livesstr;
		getline(file, livesstr);
		if (!livesstr.empty()) {			
			lives = std::stoi(livesstr);
			string result = "Loaded: " + to_string(lives) + " lives.";
			MessageBoxA(0, result.c_str(), 0, 0);
		}
		else {
			string result = "Loaded: 5 lives. You can set own count of lives in lives.txt (gd restart requires or complate some level for reseting counts)";
			MessageBoxA(0, result.c_str(), 0, 0);
		}		
		file.close();
	//FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(hModule), 0);
	return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CreateThread(0, 0x1000, Main, hModule, 0, 0);
		MH_Initialize();
		PlayLayer::mem_init();
		MH_EnableHook(MH_ALL_HOOKS);
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:	
		break;
	}
	return TRUE;
}