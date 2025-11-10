// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GLBSServiceConnector.h"
#include "Engine/DataAsset.h"
#include "ListViewSessionData.generated.h"

/**
 * 
 */
UCLASS(Blueprintable,BlueprintType)
class ODINFLEET_API UListViewSessionData : public UDataAsset
{
	GENERATED_BODY()

protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ListViewSessionData")
	FGameSessionData SessionData;
};
