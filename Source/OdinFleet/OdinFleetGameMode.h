// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "OdinFleetGameMode.generated.h"

/**
 *  Simple GameMode for a first person game
 */

struct FProcessParameters;

DECLARE_LOG_CATEGORY_EXTERN(GameServerLog, Log, All);

UCLASS(MinimalAPI)
class AOdinFleetGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AOdinFleetGameMode();

protected:
	virtual void BeginPlay() override;

private:
	void InitGameLift();

private:
	TSharedPtr<FProcessParameters> ProcessParameters;
};



