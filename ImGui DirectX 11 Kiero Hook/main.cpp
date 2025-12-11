#include "includes.h"
#include "inventory_p.h"
#include "skins.h"
void Unload() {
	printf("[+] Starting DLL unload sequence...\n");
	
	// Set unload flag to trigger DirectX cleanup
	SDK::shouldUnload = true;
	
	// Wait a bit for DirectX cleanup to complete
	Sleep(100);
	
	S::Shutdown();
	H::Destroy();
	
	// Shutdown logger
	logger::shutdown();
	
	// Shutdown kiero (this will also unhook any remaining hooks)
	kiero::shutdown();
	
	
	// Exit thread and unload DLL
	FreeLibraryAndExitThread(g_hModule, 0);
}
DWORD WINAPI MainThread(LPVOID lpReserved)
{
#ifdef _DEBUG
	logger::initialize();
#endif
	bool init_hook = false;
	bool modules_loaded = false;

	while (GetModuleHandle(L"matchmaking.dll") == nullptr) {

	}
	while (!modules_loaded) {
		if (!I::initialize()) {

			printf("[I] Failed to initialize interfaces!\n");
		}
		if (!H::Setup()) {

			printf("[H] Failed to initialize hooks!\n");
		}

		modules_loaded = true;
	}
	do {
		if (kiero::init(kiero::RenderType::D3D11) == kiero::Status::Success)
		{
			kiero::bind(8, (void**)&oPresent, hkPresent);
			kiero::bind(13, (void**)&oResizeBuffers, hkResizeBuffers);
			init_hook = true;
		}
	} while (!init_hook);
	static bool added = false;

	while (!SDK::shouldUnload) {
		Sleep(500);
		if (!Events->inited) {
			if (Events->Intilization())
				printf("[+] Event manager initialized!\n");
		}
		if (InventoryPersistence::init() && !added) {
			int restored = InventoryPersistence::LoadAndRecreate("added_items.json");
			if (restored > 0) {
				printf("[+] Restored %d items from JSON.\n", restored);
				added = true;
				InventoryPersistence::Equip();

			}
		}

		if (INVENTORY::Dump()) {
			printf("[I] Inventory dumped successfully.\n");

		}
	}

	Unload();
	return TRUE;
}

BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		g_hModule = hMod;
		DisableThreadLibraryCalls(hMod);
		CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		break;
	case DLL_PROCESS_DETACH:
		// Only call Unload if we haven't already unloaded
		if (!SDK::shouldUnload) {
			Unload();
		}
		break;
	}
	return TRUE;
}