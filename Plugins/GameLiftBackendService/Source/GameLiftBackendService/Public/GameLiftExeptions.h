// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/UserDefinedEnum.h"
#include "GameLiftExeptions.generated.h"

/**
 * 
 */
UCLASS()
class GAMELIFTBACKENDSERVICE_API UGameLiftExeptions : public UUserDefinedEnum
{
	GENERATED_BODY()
};

UENUM(BlueprintType,Blueprintable)
enum EGameLiftExceptions
{
	NONE = 0 UMETA(DisplayName = "None"),
	EXCEPTION = 400 UMETA(DisplayName = "Exception"),
	FLEET_CAPACITY_EXCEPTION = 403 UMETA(DisplayName = "Fleet Capacity Exception"),
};
UENUM(BlueprintType)
enum class EGameLiftExceptionsBP :uint8
{
	None,
	Exception,
	Fleet_capacity_Exception,
};