#include "DroneCompanionRuntimeModule.h"

DEFINE_LOG_CATEGORY(LogDroneCompanion);

void FDroneCompanionRuntimeModule::StartupModule()
{
	UE_LOG(LogDroneCompanion, Log, TEXT("DroneCompanionRuntime module started."));
}

void FDroneCompanionRuntimeModule::ShutdownModule()
{
	UE_LOG(LogDroneCompanion, Log, TEXT("DroneCompanionRuntime module shut down."));
}

IMPLEMENT_MODULE(FDroneCompanionRuntimeModule, DroneCompanionRuntime)
