// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GLBSServiceConnector.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType,Blueprintable)
struct FGameSessionData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString GameSessionId;
	UPROPERTY(BlueprintReadWrite)
	FString GameSessionName;
	UPROPERTY(BlueprintReadWrite)
	FString FleetId;
	UPROPERTY(BlueprintReadWrite)
	int32 CreationTime;
	UPROPERTY(BlueprintReadWrite)
	int32 TerminationTime;
	UPROPERTY(BlueprintReadWrite)
	int32 CurrentPlayerCount;
	UPROPERTY(BlueprintReadWrite)
	int32 MaxPlayerCount;
	UPROPERTY(BlueprintReadWrite)
	FString Status;
	UPROPERTY(BlueprintReadWrite)
	FString StatusReason;
	UPROPERTY(BlueprintReadWrite)
	FString IpAddress;
	UPROPERTY(BlueprintReadWrite)
	FString DnsName;
	UPROPERTY(BlueprintReadWrite)
	int32 Port;
	UPROPERTY(BlueprintReadWrite)
	FString CreatorId;
	UPROPERTY(BlueprintReadWrite)
	FString Location;
};
USTRUCT(BlueprintType,Blueprintable)
struct FPlayerSessionData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite)
	FString PlayerSessionId;
	UPROPERTY(BlueprintReadWrite)
	FString PlayerId;
	UPROPERTY(BlueprintReadWrite)
	FString GameSessionId;
	UPROPERTY(BlueprintReadWrite)
	FString FleetId;
	UPROPERTY(BlueprintReadWrite)
	FString FleetArn;
	UPROPERTY(BlueprintReadWrite)
	int32 CreationTime;
	UPROPERTY(BlueprintReadWrite)
	int32 TerminationTime;
	UPROPERTY(BlueprintReadWrite)
	FString Status;
	UPROPERTY(BlueprintReadWrite)
	FString IpAddress;
	UPROPERTY(BlueprintReadWrite)
	FString DnsName;
	UPROPERTY(BlueprintReadWrite)
	int32 Port;
	UPROPERTY(BlueprintReadWrite)
	FString PlayerData;
};



DECLARE_DYNAMIC_DELEGATE_OneParam(FSearchComplete,const TArray<FGameSessionData>&, GameSessions);
DECLARE_DYNAMIC_DELEGATE_OneParam(FSingleGameSessionResult,FGameSessionData, GameSession);
DECLARE_DYNAMIC_DELEGATE_OneParam(FPlayerSessionResult,FPlayerSessionData, GameSession);
UCLASS()
class GAMELIFTBACKENDSERVICE_API UGLBSServiceConnector : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "GLBS")
	static void GetSessions(FSearchComplete OnReady);

	UFUNCTION(BlueprintCallable, Category = "GLBS")
	static void CreateGameSession(FSingleGameSessionResult OnCreated,FString CreatorId,FString GameSessionName);

	UFUNCTION(BlueprintCallable, Category = "GLBS")
	static void CreatePlayerSession(FPlayerSessionResult OnCreated,FString GameSessionId,FString PlayerId);

	UFUNCTION(BlueprintCallable, Category = "GLBS")
	static void CloseGameSession(FSingleGameSessionResult OnCreated,FString GameSessionId);
	
private:
	TWeakObjectPtr<UWorld> WorldPtr;

	static FGameSessionData CreateGameSessionFromJson(TSharedPtr<FJsonValue> GameSessionJson);
	static FPlayerSessionData CreatePlayerSessionFromJson(TSharedPtr<FJsonValue> PlayerSessionJson);
};
