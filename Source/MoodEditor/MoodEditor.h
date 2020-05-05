#pragma once

#include "Modules/ModuleManager.h"
#include "Modules/ModuleInterface.h"

class FMoodEditor final : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static inline FMoodEditor& Get()
	{
		return FModuleManager::LoadModuleChecked<FMoodEditor>("MoodEditorModule");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("MoodEditorModule");
	}
};
