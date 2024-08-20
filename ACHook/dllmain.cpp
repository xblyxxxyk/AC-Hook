#include <Windows.h>
#include <vector>
#include <cstdint>
#include <iostream>
#include "include/imgui_hook.h"
#include "include/imgui/imgui.h"
#include <algorithm>
#include <ctime>
DWORD modulebase = (DWORD)GetModuleHandle(L"ac_client.exe");
DWORD GetPointerAddress(DWORD ptr, std::vector<DWORD> offsets)
{

	DWORD addr = ptr;
	for (int i = 0; i < offsets.size(); ++i) {
		addr = *(DWORD*)addr;
		addr += offsets[i];
	}
	return addr;
}
bool showMenu = true;
bool infAmmo = false;
bool infHealth = false;
bool Minimap = true;
bool ESPBOX = true;
bool rapidfire = false;
bool noSpread = false;
bool Radar = true;
bool Aimbot = true;
bool NameESP = true;

DWORD GetProcessIdByWindowName(const std::wstring& windowName) {
	HWND hWnd = FindWindow(nullptr, windowName.c_str());
	if (hWnd == nullptr) {
		return 0;
	}

	DWORD processId = 0;
	GetWindowThreadProcessId(hWnd, &processId);
	return processId;
}
struct Vector3
{
	float x, y, z;

	Vector3() {};
	Vector3(const float x, const float y, const float z) : x(x), y(y), z(z) {}
	Vector3 operator + (const Vector3& rhs) const { return Vector3(x + rhs.x, y + rhs.y, z + rhs.z); }
	Vector3 operator - (const Vector3& rhs) const { return Vector3(x - rhs.x, y - rhs.y, z - rhs.z); }
	Vector3 operator * (const float& rhs) const { return Vector3(x * rhs, y * rhs, z * rhs); }
	Vector3 operator / (const float& rhs) const { return Vector3(x / rhs, y / rhs, z / rhs); }
	Vector3& operator += (const Vector3& rhs) { return *this = *this + rhs; }
	Vector3& operator -= (const Vector3& rhs) { return *this = *this - rhs; }
	Vector3& operator *= (const float& rhs) { return *this = *this * rhs; }
	Vector3& operator /= (const float& rhs) { return *this = *this / rhs; }
	float Length() const { return sqrtf(x * x + y * y + z * z); }
	Vector3 Normalize() const { return *this * (1 / Length()); }
	float Distance(const Vector3& rhs) const { return (*this - rhs).Length(); }


};
struct Vector4
{
	float x, y, z, w;
};
struct Vec2
{
	float x, y;
};
using vec3 = Vector3;
using vec4 = Vector4;
using Vec3 = Vector3;
using Vec4 = Vector4;
int windowWidth = 1280; //put ur own res if u want
int windowHeight = 1024;

bool WorldToScreen(vec3 pos, vec3& screen, float matrix[16], int windowWidth, int windowHeight)
{
	vec4 clipCoords;
	clipCoords.x = pos.x * matrix[0] + pos.y * matrix[4] + pos.z * matrix[8] + matrix[12];
	clipCoords.y = pos.x * matrix[1] + pos.y * matrix[5] + pos.z * matrix[9] + matrix[13];
	clipCoords.z = pos.x * matrix[2] + pos.y * matrix[6] + pos.z * matrix[10] + matrix[14];
	clipCoords.w = pos.x * matrix[3] + pos.y * matrix[7] + pos.z * matrix[11] + matrix[15];

	if (clipCoords.w < 0.1f)
		return false;

	vec3 NDC{ clipCoords.x / clipCoords.w, clipCoords.y / clipCoords.w, clipCoords.z / clipCoords.w };

	screen.x = (windowWidth / 2.0 * NDC.x) + (NDC.x + windowWidth / 2.0);
	screen.y = -(windowHeight / 2.0 * NDC.y) + (NDC.y + windowHeight / 2.0);

	return true;
}

std::wstring ConvertToWideString(const char* str) {
	int size_needed = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
	std::wstring wstr(size_needed, 0);
	MultiByteToWideChar(CP_ACP, 0, str, -1, &wstr[0], size_needed);
	return wstr;
}

bool GetWindowDimensions(const char* processName, int& width, int& height) {

	std::wstring wProcessName = ConvertToWideString(processName);

	HWND hwnd = FindWindow(NULL, wProcessName.c_str());
	if (hwnd == NULL) {
		return false;
	}

	RECT rect;
	if (GetClientRect(hwnd, &rect)) {
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
		return true;
	}
	else {
		return false;
	}
}
vec3 centerScreenPos = { static_cast<float>(windowWidth) / 2, static_cast<float>(windowHeight) / 2, 0 }; //call GetRes() at start of function or will stay at 1280x1024//hwdp
void GetRes() {
	if (GetWindowDimensions("AssaultCube", windowWidth, windowHeight)) {
		centerScreenPos = { static_cast<float>(windowWidth) / 2, static_cast<float>(windowHeight) / 2, 0 }; //call GetRes() at start of function or will stay at 1280x1024//hwdp
		//this is called for aimbot and esp seperately
	}
	else {
		//dobre chlopaki patrza na swiat zza krat
	}
}
vec3 WorldToScreen(vec3 pos, float matrix[16], int windowWidth, int windowHeight)
{
	vec4 clipCoords;
	clipCoords.x = pos.x * matrix[0] + pos.y * matrix[4] + pos.z * matrix[8] + matrix[12];
	clipCoords.y = pos.x * matrix[1] + pos.y * matrix[5] + pos.z * matrix[9] + matrix[13];
	clipCoords.z = pos.x * matrix[2] + pos.y * matrix[6] + pos.z * matrix[10] + matrix[14];
	clipCoords.w = pos.x * matrix[3] + pos.y * matrix[7] + pos.z * matrix[11] + matrix[15];

	if (clipCoords.w < 0.1f)
		return vec3{ 0.0f, 0.0f, 0.0f };

	vec3 NDC{ clipCoords.x / clipCoords.w, clipCoords.y / clipCoords.w, clipCoords.z / clipCoords.w };
}
extern float* ViewMatrix = (float*)(0x57DFD0);
vec3 WorldToScreen(vec3& pos) { //we use this to only pass 1 argument its easier
	vec3 screen;

	if (WorldToScreen(pos, screen, ViewMatrix, windowWidth, windowHeight))
		return screen;
	return vec3(0, 0, 0);
}

DWORD processId = GetProcessIdByWindowName(L"AssaultCube");

namespace Settings {
	namespace ESP {
		extern bool enabled = true;
		extern bool drawTeam = false;
		extern ImColor* teamColor = new ImColor(0, 0, 255, 255);
		extern ImColor* enemyColor = new ImColor(255, 0, 0, 255);
	}
	namespace Aimbot {
		extern bool enabled = true;
		extern bool smoothing = true;
		extern float smoothingAmount = 1.0f; //put smoothing here
		extern bool checkInFov = true; // should we check whats inside fov or should fov be useless?
		extern float fov = 180; // actual current fov
		extern bool drawFovCircle = true;
	}

}

class Player //might not be fully correct, dump your own player struct if you want
{
public:
	void* vtable;
	Vector3 o; //0x0008
	Vector3 velocity; //0x0010
	Vector3 N00000184; //0x001C
	Vector3 pos; //0x0028
	float yaw; //0x0034
	float pitch; //0x0038
	float roll; //0x003C
	char pad_0040[16]; //0x0040
	float eyeHeight; //0x000C
	char pad_0054[152]; //0x0054
	int32_t health; //0x00EC
	char pad_00F0[277]; //0x00F0
	char name[104]; //0x0205
	char pad_026D[159]; //0x026D
	int8_t team; //0x030C
};
//extern float* ViewMatrix = (float*)(0x57DFD0); correct for 1.3.0.2
class EntityList {
public:
	Player* players[64];
};
extern Player* localPlayerPtr = *(Player**)(0x57E0A8);
extern int numPlayers = *(int*)(0x58AC0C); //all correct & tested on 1.3.0.2
extern EntityList* players = *(EntityList**)(0x58AC04);

bool PatchMemory(HANDLE hProcess, DWORD address, BYTE* data, SIZE_T dataSize) {
	DWORD oldProtect;

	if (!VirtualProtectEx(hProcess, (LPVOID)address, dataSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
		return false;
	}

	SIZE_T bytesWritten;
	if (!WriteProcessMemory(hProcess, (LPVOID)address, data, dataSize, &bytesWritten) || bytesWritten != dataSize) {
		VirtualProtectEx(hProcess, (LPVOID)address, dataSize, oldProtect, &oldProtect);
		return false;
	}

	if (!VirtualProtectEx(hProcess, (LPVOID)address, dataSize, oldProtect, &oldProtect)) {
		return false;
	}

	return true;
}
void NOPInstruction(BYTE* targetAddress, size_t numNOPs) {
	DWORD oldProtection; // we can use this to nop
	VirtualProtect(targetAddress, numNOPs, PAGE_EXECUTE_READWRITE, &oldProtection);

	for (size_t i = 0; i < numNOPs; ++i) {
		targetAddress[i] = 0x90;
	}

	VirtualProtect(targetAddress, numNOPs, oldProtection, &oldProtection);
}
void nopRapid() { //nop for rapid trash solution, make ur own rapid slider instead of nopping
	if (modulebase == 0) return;

	DWORD staticPointerOffset = 0x0017E0A8;
	DWORD offset = 0x164;

	DWORD* staticPointerAddress = reinterpret_cast<DWORD*>(modulebase + staticPointerOffset);
	DWORD finalAddress = *staticPointerAddress + offset;

	BYTE* targetAddress = reinterpret_cast<BYTE*>(finalAddress);
	NOPInstruction(targetAddress, 5);
}

bool IsKeyPressed(int key)
{
	return GetAsyncKeyState(key) & 0x8000;
}

void drawCenteredText(std::string text, float x, float y) {
	float textWidth = ImGui::CalcTextSize(text.c_str()).x;
	ImGui::GetBackgroundDrawList()->AddText(ImVec2(x - textWidth / 2, y), IM_COL32(255, 255, 255, 255), text.c_str());
}

void normalizeAngle(vec3& angle) {
	if (angle.x > 360)
		angle.x -= 360;
	if (angle.x < 0)
		angle.x += 360;
	if (angle.y > 90)
		angle.y -= 90;
	if (angle.y < -90)
		angle.y += 90;
}
inline float RAD2DEG(float rad)
{
	return rad * 57.295780181884765625f;
}
struct Angle
{
	float yaw = 0.f, pitch = 0.f, roll = 0.f;
	Angle operator+(const Angle& v) const
	{
		return { yaw + v.yaw, pitch + v.pitch,roll + v.roll };
	}
	Angle operator-(const Angle& v) const
	{
		return { yaw - v.yaw,pitch - v.pitch, roll - v.roll };
	}
};
vec3 Subtract(vec3 src, vec3 dst)
{
	vec3 diff;
	diff.x = src.x - dst.x;
	diff.y = src.y - dst.y;
	diff.z = src.z - dst.z;
	return diff;
}

float Magnitude(Vec3 vec)
{
	return sqrtf(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

float Distance(vec3 src, vec3 dst)
{
	vec3 diff = Subtract(src, dst);
	return Magnitude(diff);
}
float PI = 3.141592653589793238462643383279502884197;
vec3 CalcAngle(vec3 src, vec3 dst)
{
	vec3 angle;
	angle.x = -atan2f(dst.x - src.x, dst.y - src.y) / PI * 180.0f + 180.0f;
	angle.y = asinf((dst.z - src.z) / Distance(src, dst)) * 180.0f / PI;
	angle.z = 0.0f;

	return angle;
}
void drawScalingBarVertical(float x1, float y1, float x2, float y2, float width, float value, float max, ImColor color) {

	float heightDiff = y2 - y1;

	if (value < 0) value = 0;
	if (value > max) value = max;

	float scaledHeight = heightDiff * (value / max);

	ImGui::GetBackgroundDrawList()->AddRect(ImVec2(x1, y1), ImVec2(x2, y2), IM_COL32(200, 200, 200, 0), 0, 0, width);

	ImGui::GetBackgroundDrawList()->AddRectFilled(ImVec2(x1, y2 - scaledHeight), ImVec2(x2, y2), color);
}



int FOV = 90; // for shit estimate method

bool isInFovWS2(vec3& screenLoc) {
	if (abs(centerScreenPos.Distance(screenLoc)) < Settings::Aimbot::fov)
		return true;
	return false;
}
/* kinda trash method but use if you want
bool isInFov(Player* owner, Vec3 looking) {
	vec3 angle = CalcAngle(owner->pos, looking);
	vec3 playerAngle(owner->yaw + 180, owner->pitch, 0);
	normalizeAngle(playerAngle);

	vec3 angleDiff = playerAngle - angle;
	normalizeAngle(angleDiff);
	return (fabs(angleDiff.x) <= FOV / 2 && fabs(angleDiff.y) <= FOV / 2);
}
*/
/* ESTIMATE GARBAGE V2 USE VIEW MATRIX NOT THIS SHIT
Player* getNearestPlayer() {
	Player* nearestPlayer = nullptr;
	float nearestDistance = 9999999.0f;
	for (int i = 1; i < numPlayers + 1; i++) {
		Player* player = players->players[i];
		float distance = localPlayerPtr->pos.Distance(player->pos);
		if (distance < nearestDistance) {
			nearestDistance = distance;
			nearestPlayer = player;
		}
		return nearestPlayer;
	}
}
*/
/* NO VIEWMATRIX JUST ESTIMATE GARBAGE
Player* getNearestEntityAngle() {
	vec3 playerAngle{ localPlayerPtr->yaw + 180, localPlayerPtr->pitch, 0 };
	normalizeAngle(playerAngle);
	Player* nearestPlayer = nullptr;
	float smallestangle = 9999999.0f;
	bool teammate = false;
	for (int i = 1; i < numPlayers + 1; i++) {
		Player* player = players->players[i];
		if (IsBadReadPtr(player, sizeof(Player))) {
			continue;
		}
		teammate = player->team == localPlayerPtr->team;
		if (player->health > 100 || player->health <= 0)
			continue;
		/*if (teammate && !Settings::ESP::drawTeam)
			continue;
		*/
		/*
				vec3 targetAngle = CalcAngle(localPlayerPtr->pos, player->pos);
				vec3 angleDiff = playerAngle - targetAngle;
				normalizeAngle(angleDiff);
				float angleMagnitude = angleDiff.Length();
				if (angleMagnitude < smallestangle) {
					smallestangle = angleMagnitude;
					nearestPlayer = player;
				}
			}
			return nearestPlayer;
		}
		*/
float curAimTime = 0;
clock_t lastAimTime = clock();
Player* curTarget = nullptr;

#define min(a,b) a < b ? a : b

Player* getNearestEntityWS2() {
	Player* nearestPlayer = nullptr;
	float nearestDistance = 9999999.0f;
	float distance = 0;
	for (int i = 0; i < numPlayers; i++) {
		Player* player = players->players[i];
		if (IsBadReadPtr(player, sizeof(Player))) {
			continue;
		}
		if (player->health > 100 || player->health <= 0 || player->team == localPlayerPtr->team)
			continue;
		vec3 headPos = { player->o.x, player->o.y, player->o.z };
		vec3 headScreenPos = WorldToScreen(headPos);

		if (Settings::Aimbot::checkInFov && !isInFovWS2(headScreenPos))
			continue;
		headScreenPos.z = 0;
		distance = abs(centerScreenPos.Distance(headScreenPos));
		if (distance < nearestDistance) {
			nearestDistance = distance;
			nearestPlayer = player;
		}

	}
	return nearestPlayer;
}

void smoothAngle(vec3& from, vec3& to, float percent) { //for smoothing
	vec3 delta = to - from;
	normalizeAngle(delta);
	if (delta.x > 180) delta.x -= 360;
	if (delta.x < 180) delta.x += 360;
	if (delta.y > 45) delta.y -= 45;
	if (delta.y < -45) delta.y += 45;
	from.x += delta.x * percent;
	from.y += delta.y * percent;
	normalizeAngle(from);
}
void aimAt() {
	GetRes();
	if (Settings::Aimbot::drawFovCircle)
		ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(centerScreenPos.x, centerScreenPos.y), Settings::Aimbot::fov, IM_COL32(255, 255, 255, 128), 100);

	if (!Settings::Aimbot::enabled || !GetAsyncKeyState(VK_SHIFT)) {
		curAimTime = 0;
		lastAimTime = clock();
		curTarget = nullptr;
		return;
	}

	Player* target = getNearestEntityWS2();

	if (!target)
		return;
	if (target != curTarget) {
		curAimTime = 0;
		lastAimTime = clock();
		curTarget = target;
	}
	clock_t now = clock();
	curAimTime + static_cast<float>(now - lastAimTime) / CLOCKS_PER_SEC;
	lastAimTime = now;
	float percent = min(curAimTime / Settings::Aimbot::smoothingAmount, 1);
	Vec3 targetAngle = CalcAngle(localPlayerPtr->pos, target->pos);
	targetAngle.x += 0;

	vec3 currentAngle = { localPlayerPtr->yaw, localPlayerPtr->pitch, 0 };

	if (Settings::Aimbot::smoothing) {
		if (percent >= 1) {
			curAimTime = 0;
			percent = 1;
		}
		smoothAngle(currentAngle, targetAngle, percent);
	}
	else
	{
		currentAngle = targetAngle;
	}
	localPlayerPtr->yaw = targetAngle.x;
	localPlayerPtr->pitch = targetAngle.y;

}

void DrawESP() {
	bool teammate = false;
	GetRes();
	for (int i = 0; i < numPlayers; i++) {
		Player* player = players->players[i];
		if (!player) continue;


		if (IsBadReadPtr(player, sizeof(Player))) {
			continue;
		}

		teammate = player->team == localPlayerPtr->team;
		if (player->health > 100 || player->health <= 0)
			continue;
		if (teammate && !Settings::ESP::drawTeam)
			continue;

		vec3 headPos = { player->o.x, player->o.y, player->o.z };
		vec3 feetPos = { player->o.x, player->o.y, player->o.z - player->eyeHeight };

		vec3 headScreenPos = WorldToScreen(headPos);
		vec3 feetScreenPos = WorldToScreen(feetPos);

		float height = abs(headScreenPos.y - feetScreenPos.y);
		float width = height / 4;

		ImVec2 topLeft = ImVec2(feetScreenPos.x - width, headScreenPos.y);
		ImVec2 topRight = ImVec2(feetScreenPos.x + width, headScreenPos.y);

		ImVec2 bottomRight = ImVec2(feetScreenPos.x + width, feetScreenPos.y);
		ImVec2 bottomLeft = ImVec2(feetScreenPos.x - width, feetScreenPos.y);
		ImColor espColor = teammate ? *Settings::ESP::teamColor : *Settings::ESP::enemyColor;
		ImGui::GetBackgroundDrawList()->AddQuad(topLeft, bottomLeft, bottomRight, topRight, espColor);

		if (NameESP) {
			drawCenteredText(player->name, feetScreenPos.x, feetScreenPos.y + 5);
		}
		//HEALTH ESP(needs fix)-> drawScalingBarVertical(bottomLeft.x -5, - width / 2, bottomLeft.y, bottomLeft.x-5, topRight.y, 1, 100, ImColor(255, 0, 0, 255));
	}
}

bool wasKeyDown = false;

DWORD WINAPI UnloadThread(LPVOID lpParameter) {
	// Perform clean-up operations
	ImGuiHook::Unload();

	HMODULE hModule = GetModuleHandle(NULL);
	if (hModule) {
		FreeLibrary(hModule);
	}
	return 0;
}
void ModuleRageQuit() {
	HANDLE hThread = CreateThread
	(
		NULL, 
		0, 
		UnloadThread,
		NULL,
		0,   
		NULL
	);

	if (hThread) {
		CloseHandle(hThread); //after u unload well cant inject again with process hacker sad :(
	}
}
void WhyYouHoldTheButtonAndItKeepsUpdatingWhenItShouldWorkByJustPressingItOnceInstead() {
	SHORT keyState = GetAsyncKeyState(VK_INSERT);
	bool isKeyDown = (keyState & 0x8000) != 0;

	if (isKeyDown && !wasKeyDown) {
		showMenu = !showMenu;
	}

	wasKeyDown = isKeyDown;
}

void RenderMain()
{

	WhyYouHoldTheButtonAndItKeepsUpdatingWhenItShouldWorkByJustPressingItOnceInstead();

	if (showMenu)
	{
		ImGui::CaptureMouseFromApp(false);
		ImGui::SetNextWindowSize(ImVec2(400, 400));
		ImGui::Begin("AcHOOK by blyscyk [INSERT]");
		ImGui::Spacing();
		ImGui::Text("Base: 0x%08X", modulebase);
		ImGui::Spacing();
		ImGui::Checkbox("Aimbot [SHIFT]", &Aimbot);
		//dont need but u can fix if you want ImGui::SliderFloat("Smoothing", &Settings::Aimbot::smoothingAmount, 0.1f, 100.0f);
		ImGui::Checkbox("FOV Circle", &Settings::Aimbot::drawFovCircle);
		ImGui::SliderFloat("FOV Size", &Settings::Aimbot::fov, 50.0f, 500.0f);
		ImGui::Checkbox("Box ESP", &ESPBOX);
		ImGui::Checkbox("Name ESP", &NameESP);
		ImGui::Checkbox("Infinite Ammo", &infAmmo);
		ImGui::Checkbox("Infinite Health", &infHealth);
		ImGui::Checkbox("Rapid Fire", &rapidfire);
		ImGui::Checkbox("No Spread", &noSpread);
		ImGui::Checkbox("Minimap ESP", &Minimap);
		ImGui::Checkbox("Radar ESP", &Radar);

		ImGui::Text("Use Windows to select Functions");
		ImGui::Text("No ESP? Inject in a gamemode with players.");
		ImGui::Text("Made by blyscyk 2024-2024");
		if (ImGui::Button("Unload Dll")) {
			ModuleRageQuit();
		}

	}

	if (infAmmo) {
		int* ammo = (int*)GetPointerAddress(modulebase + 0x17E0A8, { 0x140 });
		int* ammo2 = (int*)GetPointerAddress(modulebase + 0x17E0A8, { 0x12C });

		*ammo = 1337;
		*ammo2 = 1337;
	}

	if (ESPBOX) {
		DrawESP();
	}
	if (Aimbot) {
		aimAt();
	}
	if (infHealth) {
		int* health = (int*)GetPointerAddress(modulebase + 0x17E0A8, { 0xEC });
		*health = 1337;
	}
	if (rapidfire) {
		DWORD* ACWORLD = reinterpret_cast<DWORD*>(modulebase + 0x17E0A8);
		BYTE* writerapid1 = reinterpret_cast<BYTE*>(*ACWORLD + 0x164);
		BYTE* writerapid2 = reinterpret_cast<BYTE*>(0x4C73EA);
		NOPInstruction(writerapid1, 5);
		NOPInstruction(writerapid2, 2);
	}

	if (Minimap) {
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
		//we jump to the skip and then nop last byte so it fits
		DWORD offset = 0x45D26B;
		BYTE patchData[6] = { 0xE9, 0x76, 0x01, 0x00, 0x00, 0x90 };

		if (!PatchMemory(hProcess, offset, patchData, sizeof(patchData))) {
			// hwdp 1337 //
		}
		CloseHandle(hProcess);
	}

	if (Radar) {
		HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);

		//same as above
		DWORD offset = 0x45C479;
		BYTE patchData[6] = { 0xE9, 0x88, 0x01, 0x00, 0x00, 0x90 };

		if (!PatchMemory(hProcess, offset, patchData, sizeof(patchData))) {
			// jebac kapusi pdw chada nie zmienia zdania //
		}
	}
	if (noSpread) {
		//simple just fully nop it, use this address for lower spread aswell
		BYTE* spread = reinterpret_cast<BYTE*>(0x4C8E52);
		NOPInstruction(spread, 5);
	}
}


BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hMod);
		ImGuiHook::Load(RenderMain); //trash kiero hook :) dont use medal overlay or it wont even show menu lol
		break;
	case DLL_PROCESS_DETACH:
		ImGuiHook::Unload();
		break;
	}
	return TRUE;
}