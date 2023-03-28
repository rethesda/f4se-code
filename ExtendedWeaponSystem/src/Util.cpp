#include "Util.h"

char tempbuf[8192] = { 0 };
char* _MESSAGE(const char* fmt, ...) {
	va_list args;

	va_start(args, fmt);
	vsnprintf(tempbuf, sizeof(tempbuf), fmt, args);
	va_end(args);
	spdlog::log(spdlog::level::info, tempbuf);

	return tempbuf;
}

void Dump(void* mem, unsigned int size) {
	char* p = static_cast<char*>(mem);
	unsigned char* up = (unsigned char*)p;
	std::stringstream stream;
	int row = 0;
	for (unsigned int i = 0; i < size; i++) {
		stream << std::setfill('0') << std::setw(2) << std::hex << (int)up[i] << " ";
		if (i % 8 == 7) {
			stream << "\t0x"
				   << std::setw(2) << std::hex << (int)up[i]
				   << std::setw(2) << (int)up[i - 1]
				   << std::setw(2) << (int)up[i - 2]
				   << std::setw(2) << (int)up[i - 3]
				   << std::setw(2) << (int)up[i - 4]
				   << std::setw(2) << (int)up[i - 5]
				   << std::setw(2) << (int)up[i - 6]
				   << std::setw(2) << (int)up[i - 7] << std::setfill('0');
			stream << "\t0x" << std::setw(2) << std::hex << row * 8 << std::setfill('0');
			_MESSAGE("%s", stream.str().c_str());
			stream.str(std::string());
			row++;
		}
	}
}

BSTEventSource<void*>* GetGlobalEventSource(BSTGlobalEvent_OLD* globalEvents, const char* globalName) {
	auto sources = (globalEvents->eventSources);
	for (auto elem = sources.begin(); elem != sources.end(); ++elem) {
		const char* name = GetObjectClassName(elem) + 15;
		if (globalName == name) {
			return (BSTEventSource<void*>*)&((elem));
		}
	}
	return nullptr;
}

const char* GetObjectClassNameImpl(const char* result, void* objBase) {
	using namespace RTTI;
	void** obj = (void**)objBase;
	CompleteObjectLocator** vtbl = (CompleteObjectLocator**)obj[0];
	CompleteObjectLocator* rtti = vtbl[-1];
	RVA<TypeDescriptor> typeDesc = rtti->typeDescriptor;

	// starts with ,?
	const char* _name = typeDesc->mangled_name();
	if ((_name[0] == '.') && (_name[1] == '?')) {
		// is at most 100 chars long
		for (uint32_t i = 0; i < 100; i++) {
			if (_name[i] == 0) {
				// remove the .?AV
				return _name + 4;
				break;
			}
		}
	}
	return result;
}

const char* GetObjectClassName(void* objBase) {
	const char* result = "<no rtti>";
	__try {
		result = GetObjectClassNameImpl(result, objBase);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		// return the default
	}

	return result;
}

bool IsButtonPressed(ButtonEvent* btnEvent) {
	if (btnEvent->value == BUTTON_UP && (btnEvent->heldDownSecs > 0 && btnEvent->heldDownSecs < BUTTON_HOLD_TIMER)) {
		return true;
	}
	return false;
}

bool IsHoldingButton(ButtonEvent* btnEvent) {
	if (btnEvent->value == BUTTON_DOWN && btnEvent->heldDownSecs >= BUTTON_HOLD_TIMER) {
		return true;
	}
	return false;
}

bool IsPlayerInFirstPerson() {
	if (!PlayerCamera::GetSingleton()) {
		return false;
	}
	return PlayerCamera::GetSingleton()->currentState == PlayerCamera::GetSingleton()->cameraStates[CameraState::kFirstPerson];
}

bool IsPlayerInThirdPerson() {
	if (!PlayerCamera::GetSingleton()) {
		return false;
	}
	return PlayerCamera::GetSingleton()->currentState == PlayerCamera::GetSingleton()->cameraStates[CameraState::k3rdPerson];
}

bool IsPlayerSprinting() {
	return PlayerCharacter::GetSingleton()->ActorState::moveMode & 0x0100;
}

bool IsPlayerWeaponDrawn() {
	return PlayerCharacter::GetSingleton()->GetWeaponMagicDrawn();
}

bool IsPlayerWeaponReloadable() {
	PlayerCharacter::GetSingleton()->currentProcess->GetEquippedItemArrayLock()->lock();
	const BSTArray<EquippedItem> equipDataArr = *PlayerCharacter::GetSingleton()->currentProcess->GetEquippedItemArray();
	if (equipDataArr.empty()) {
		return false;
	}

	const EquippedItem* equipData = nullptr;
	for (auto& elem : equipDataArr) {
		uint32_t equipIndex = elem.equipIndex.index;
		if (equipIndex == EquipIndex::kDefault) {
			equipData = &elem;
			break;
		}
	}
	PlayerCharacter::GetSingleton()->currentProcess->GetEquippedItemArrayLock()->unlock();
	if (!equipData) {
		return false;
	}

	IsReloadableDataWrapper wrapper = { 0i64, PlayerCharacter::GetSingleton() };

	return IsWeaponReloadable(&wrapper, equipData);
}

bool IsPlayerWeaponReloading() {
	return PlayerCharacter::GetSingleton()->ActorState::gunState == GUN_STATE::kReloading;
}

bool IsPlayerWeaponThrowable() {
	return false;  //TODO
}

bool IsWeaponReloadable(IsReloadableDataWrapper* data, const EquippedItem* item) {
	using func_t = bool (*)(IsReloadableDataWrapper*, const EquippedItem*);
	REL::Relocation<func_t> func{ REL::ID(1089596) };

	return !func(data, item);
}

bool IsWeaponThrowable(uint32_t equipIndex) {
	return equipIndex == EquipIndex::kThrowable;
}

TESForm* GetFormFromIdentifier(const string& identifier) {
	auto delimiter = identifier.find('|');
	if (delimiter != string::npos) {
		string modName = identifier.substr(0, delimiter);
		string modForm = identifier.substr(delimiter + 1);

		TESDataHandler* g_dataHandler = TESDataHandler::GetSingleton();

		const TESFile* mod = (g_dataHandler)->LookupModByName(modName.c_str());
		uint8_t modIndex = mod->compileIndex;
		if (mod && modIndex != -1) {
			uint32_t formID = stoul(modForm, nullptr, 16) & 0xFFFFFF;
			uint32_t flags = GetOffset<uint32_t>(mod, 0x334);
			if (flags & (1 << 9)) {
				// ESL
				formID &= 0xFFF;
				formID |= 0xFE << 24;
				formID |= GetOffset<uint32_t>(mod, 0x372) << 12;  // ESL load order
			} else {
				formID |= (modIndex) << 24;
			}
			return TESForm::GetFormByID(formID);
		}
	}
	return nullptr;
}

//Gather the forms we need
bool GetForms() {
	bool toReturn = true;
	weaponHasSequentialReloadKeyword = GetFormFromIdentifier("ExtendedWeaponSystem.esm|1ED4")->As<BGSKeyword>();
	if (!weaponHasSequentialReloadKeyword) {
		logError("Unable to get weaponHasSequentialReloadKeyword, you are lacking some file/files");
	}
	weaponHasSpeedReloadKeyword = GetFormFromIdentifier("ExtendedWeaponSystem.esm|ECBD")->As<BGSKeyword>();
	if (!weaponHasSpeedReloadKeyword) {
		logError("Unable to get weaponHasSpeedReloadKeyword, you are lacking some file/files");
	}
	weaponHasThermalScopeKeyword = GetFormFromIdentifier("Code_SharedAttachments.esm|6436")->As<BGSKeyword>();
	if (!weaponHasThermalScopeKeyword) {
		logError("Unable to get weaponHasThermalScopeKeyword, you are lacking some file/files");
	}
	weaponIsClosedBoltKeyword = GetFormFromIdentifier("ExtendedWeaponSystem.esm|1ED8")->As<BGSKeyword>();
	if (!weaponIsClosedBoltKeyword) {
		logError("Unable to get weaponIsClosedBoltKeyword, you are lacking some file/files");
	}
	weaponIsOpenBoltKeyword = GetFormFromIdentifier("ExtendedWeaponSystem.esm|1ED9")->As<BGSKeyword>();
	if (!weaponIsOpenBoltKeyword) {
		logError("Unable to get weaponIsOpenBoltKeyword, you are lacking some file/files");
	}
	//ThermalFXS = GetFormFromIdentifier("Code_SharedAttachments.esm|5BCB")->As<BGSKeyword>();
	//if (!ThermalFXS) {
	//	logError("Unable to get ThermalFXS, you are lacking some file/files");
	//}
	if (!weaponHasSequentialReloadKeyword || !weaponHasThermalScopeKeyword || !weaponIsClosedBoltKeyword || !weaponIsOpenBoltKeyword || !weaponHasSpeedReloadKeyword) {
		toReturn = false;
	}
	return toReturn;
}

EquippedItem& GetPlayerEquippedItemDefault() {
	PlayerCharacter::GetSingleton()->currentProcess->GetEquippedItemArrayLock()->lock();
	EquippedItem& a_item = PlayerCharacter::GetSingleton()->currentProcess->GetEquippedItemArray()->data()[EquipIndex::kDefault];
	PlayerCharacter::GetSingleton()->currentProcess->GetEquippedItemArrayLock()->unlock();
	return a_item;
}

EquippedWeapon& GetPlayerEquippedWeaponDefault() {
	PlayerCharacter::GetSingleton()->currentProcess->GetEquippedItemArrayLock()->lock();
	EquippedItem& a_item = PlayerCharacter::GetSingleton()->currentProcess->GetEquippedItemArray()->data()[EquipIndex::kDefault];
	PlayerCharacter::GetSingleton()->currentProcess->GetEquippedItemArrayLock()->unlock();
	EquippedWeapon& a_weapon = reinterpret_cast<EquippedWeapon&>(a_item);
	if (a_weapon.IsValid() && a_weapon.weaponData.get()) {
		return a_weapon;
	}
	PlayerCharacter::GetSingleton()->currentProcess->GetEquippedWeaponByIndex(BGSEquipIndex(EquipIndex::kDefault), a_weapon);
	return a_weapon;
}

const uint32_t GetPlayerInventoryObjectCount(const TESBoundObject* item) {
	return PlayerCharacter::GetSingleton()->GetInventoryObjectCount(item);
}

const NiAVObject* GetByNameFromPlayer3D(const BSFixedString& name) {
	BSFadeNode* player3D = PlayerCharacter::GetSingleton() ? PlayerCharacter::GetSingleton()->Get3D()->IsFadeNode() : nullptr;
	if (!player3D) {
		return nullptr;
	}
	NiAVObject* obj;
	obj = BSUtilities::GetObjectByName(player3D, name, true, true);
	if (!obj) {
		return nullptr;
	}
	return obj;
}

//Functions to write a simple line of text to logs

//Write info to log
void logInfo(string text) {
	logger::info(FMT_STRING("{:s}"), text);
}

void logWarn(string text) {
	logger::warn(FMT_STRING("{:s}"), text);
}

//Write error to log. Error is unexpected result but not a crash
void logError(string text) {
	logger::error(FMT_STRING("{:s}"), text);
}

//Write critical to log. Critical is something that will crash
void logCritical(string text) {
	logger::critical(FMT_STRING("{:s}"), text);
}
