#include "Global.h"

F4SE::PluginHandle g_pluginHandle = NULL;
const F4SE::MessagingInterface* g_messaging = nullptr;
const F4SE::TaskInterface* g_taskInterface = nullptr;
const F4SE::Trampoline* g_trampoline = nullptr;

PlayerCharacter* pc = nullptr;
PlayerCamera* pcam = nullptr;
PlayerControls* pcon = nullptr;

const ButtonEvent* firstButton = nullptr;
const ButtonEvent* secondButton = nullptr;
uint32_t keyPressedCount = 0;
uint32_t keyPressedLast = 0;
float keyPressedLastTime = 0.0F;

bool gameLoading = false;
bool gameLoadingSave = false;

bool isEmptyReload = false;
bool reloadHasEnded = true;
bool reloadHasStarted = false;
bool weaponHasScopeNV = false;
bool weaponHasScopePIP = false;
bool weaponHasScopeThermal = false;
bool weaponHasSequentialReload = false;
bool weaponHasSpeedReload = false;
bool weaponIsClosedBolt = false;
bool weaponIsOpenBolt = false;
bool weaponIsQueued = false;

BGSKeyword* weaponHasScopeNVKeyword = nullptr;
BGSKeyword* weaponHasScopePIPKeyword = nullptr;
BGSKeyword* weaponHasScopeThermalKeyword = nullptr;
BGSKeyword* weaponHasSequentialReloadKeyword = nullptr;
BGSKeyword* weaponHasSpeedReloadKeyword = nullptr;
BGSKeyword* weaponIsClosedBoltKeyword = nullptr;
BGSKeyword* weaponIsOpenBoltKeyword = nullptr;
