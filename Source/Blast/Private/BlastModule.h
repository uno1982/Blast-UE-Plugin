#pragma once
#include "CoreMinimal.h"

#include "Stats/Stats2.h"

#include "Modules/ModuleInterface.h"

class FBlastModule : public IModuleInterface
{
public:
	FBlastModule();
	virtual ~FBlastModule() = default;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};


DECLARE_STATS_GROUP(TEXT("Blast"), STATGROUP_Blast, STATCAT_Advanced);

