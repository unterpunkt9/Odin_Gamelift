// Copyright Epic Games, Inc. All Rights Reserved.

#include "OdinFleetGameMode.h"

#if WITH_GAMELIFT
#include "GameLiftServerSDK.h"
#endif

DEFINE_LOG_CATEGORY(GameServerLog);

AOdinFleetGameMode::AOdinFleetGameMode() :ProcessParameters(nullptr)
{
	// stub
}

void AOdinFleetGameMode::BeginPlay()
{
	Super::BeginPlay();

#if WITH_GAMELIFT
	InitGameLift();
	#endif
}

void AOdinFleetGameMode::InitGameLift()
{
#if WITH_GAMELIFT
	UE_LOG(GameServerLog, Log, TEXT("Game Lift initialized"));
	FGameLiftServerSDKModule* GameLiftServerSdkModule = &FModuleManager::LoadModuleChecked<FGameLiftServerSDKModule>(FName("GameLiftServerSDK"));

	FServerParameters ServerParameters;
	bool bIsAnywhereActive = false;
	if (FParse::Param(FCommandLine::Get(), TEXT("glAnywhere")))
	{
		bIsAnywhereActive = true;
	}
	if (bIsAnywhereActive)
	{
		FString glAnywhereWebSocketUrl = "";
		if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereWebSocketUrl="),glAnywhereWebSocketUrl))
		{
			ServerParameters.m_webSocketUrl = TCHAR_TO_UTF8(*glAnywhereWebSocketUrl);
		}

		FString glAnywhereFleetId = "";
		if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereFleetId="),glAnywhereFleetId))
		{
			ServerParameters.m_fleetId = TCHAR_TO_UTF8(*glAnywhereFleetId);
		}

		FString glAnywhereProcessId = "";
		if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereProcessId="),glAnywhereProcessId))
		{
			ServerParameters.m_processId = TCHAR_TO_UTF8(*glAnywhereProcessId);
		}else
		{
			FString TimeString = FString::FromInt(std::time(nullptr));
			FString ProcessId = "ProcessId_" +TimeString;
			ServerParameters.m_processId = TCHAR_TO_UTF8(*ProcessId);
		}

		FString glAnywhereHostId = "";
		if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereHostId="), glAnywhereHostId))
		{
			ServerParameters.m_hostId = TCHAR_TO_UTF8(*glAnywhereHostId);
		}
		
		FString glAnywhereAuthToken = "";
		if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereAuthToken="),glAnywhereAuthToken))
		{
			ServerParameters.m_authToken = TCHAR_TO_UTF8(*glAnywhereAuthToken);
		}

		FString glAnywhereAwsRegion = "";
		if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereAwsRegion="),glAnywhereAwsRegion))
		{
			ServerParameters.m_awsRegion = TCHAR_TO_UTF8(*glAnywhereAwsRegion);
		}

		FString glAnywhereAccessKey = "";
		if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereAccessKey="),glAnywhereAccessKey))
		{
			ServerParameters.m_accessKey = TCHAR_TO_UTF8(*glAnywhereAccessKey);
		}

		FString glAnywhereSecretKey = "";
		if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereSecretKey="),glAnywhereSecretKey))
		{
			ServerParameters.m_secretKey = TCHAR_TO_UTF8(*glAnywhereSecretKey);
		}

		FString glAnywhereSessionToken  = "";
		if (FParse::Value(FCommandLine::Get(), TEXT("glAnywhereSessionToken="),glAnywhereSessionToken))
		{
			ServerParameters.m_sessionToken = TCHAR_TO_UTF8(*glAnywhereSessionToken);
		}

		UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_YELLOW);
		UE_LOG(GameServerLog, Log, TEXT(">>>> WebSocket URL: %s"), *ServerParameters.m_webSocketUrl);
		UE_LOG(GameServerLog, Log, TEXT(">>>> Fleet ID: %s"), *ServerParameters.m_fleetId);
		UE_LOG(GameServerLog, Log, TEXT(">>>> Process ID: %s"), *ServerParameters.m_processId);
		UE_LOG(GameServerLog, Log, TEXT(">>>> Host ID (Compute Name): %s"), *ServerParameters.m_hostId);
		UE_LOG(GameServerLog, Log, TEXT(">>>> Auth Token: %s"), *ServerParameters.m_authToken);
		UE_LOG(GameServerLog, Log, TEXT(">>>> Aws Region: %s"), *ServerParameters.m_awsRegion);
		UE_LOG(GameServerLog, Log, TEXT(">>>> Access Key: %s"), *ServerParameters.m_accessKey);
		UE_LOG(GameServerLog, Log, TEXT(">>>> Secret Key: %s"), *ServerParameters.m_secretKey);
		UE_LOG(GameServerLog, Log, TEXT(">>>> Session Token: %s"), *ServerParameters.m_sessionToken);
		UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_NONE);
	}

	FGameLiftGenericOutcome InitSdkOutcome = GameLiftServerSdkModule->InitSDK(ServerParameters);
	if (InitSdkOutcome.IsSuccess())
	{
		UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_GREEN);
		UE_LOG(GameServerLog, Log, TEXT("GameLift InitSDK succeeded!"));
		UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_NONE);
	}else
	{
		UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_RED);
		UE_LOG(GameServerLog, Log, TEXT("ERROR: InitSDK failed : ("));
		FGameLiftError GameLiftError = InitSdkOutcome.GetError();
		UE_LOG(GameServerLog, Log, TEXT("ERROR: %s"), *GameLiftError.m_errorMessage);
		UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_NONE);
		return;
	}
	ProcessParameters = MakeShared<FProcessParameters>();

	ProcessParameters->OnStartGameSession.BindLambda([=](Aws::GameLift::Server::Model::GameSession InGameSession)
	{
		FString GameSessionId = FString(InGameSession.GetGameSessionId());
		UE_LOG(GameServerLog, Log, TEXT("GameSession Initializing: %s"), *GameSessionId);
		GameLiftServerSdkModule->ActivateGameSession();
	});
	ProcessParameters->OnUpdateGameSession.BindLambda([=](Aws::GameLift::Server::Model::UpdateGameSession InGameSession)
	{
		UE_LOG(GameServerLog, Log, TEXT("Game SessionUpdating"));
		Aws::GameLift::Server::Model::UpdateReason c = InGameSession.GetUpdateReason();
		Aws::GameLift::Server::Model::GameSession r = InGameSession.GetGameSession();
		return;
	});
	ProcessParameters->OnTerminate.BindLambda([=]()
	{
		UE_LOG(GameServerLog, Log, TEXT("Game Server Process is terminating"));
		FGameLiftGenericOutcome processEndingOutcome = GameLiftServerSdkModule->ProcessEnding();

		FGameLiftGenericOutcome destroyOutcome = GameLiftServerSdkModule->Destroy();
		if (processEndingOutcome.IsSuccess() && destroyOutcome.IsSuccess())
		{
			UE_LOG(GameServerLog, Log, TEXT("Server process ending successfully"));
			FGenericPlatformMisc::RequestExit(false);
		}else{
			if (!processEndingOutcome.IsSuccess()) {
				const FGameLiftError& error = processEndingOutcome.GetError();
				UE_LOG(GameServerLog, Error, TEXT("ProcessEnding() failed. Error: %s"),
				error.m_errorMessage.IsEmpty() ? TEXT("Unknown error") : *error.m_errorMessage);
			}
			if (!destroyOutcome.IsSuccess()) {
				const FGameLiftError& error = destroyOutcome.GetError();
				UE_LOG(GameServerLog, Error, TEXT("Destroy() failed. Error: %s"),
				error.m_errorMessage.IsEmpty() ? TEXT("Unknown error") : *error.m_errorMessage);
			}
		}
		
		UE_LOG(GameServerLog, Log, TEXT("Dubuido"));

	});


	ProcessParameters->OnHealthCheck.BindLambda([=]()
	{
		UE_LOG(GameServerLog, Log, TEXT("Performing Health Check"));
		return true;
	});


	ProcessParameters->port = FURL::UrlConfig.DefaultPort;
	

	TArray<FString> CommandLineTokens;
	TArray<FString> CommandLineSwitches;

	FCommandLine::Parse(FCommandLine::Get(),CommandLineTokens,CommandLineSwitches);

	for (FString Switch : CommandLineSwitches)
	{
		FString Key;
		FString Value;

		if (Switch.Split("=",&Key,&Value))
		{
		    UE_LOG(GameServerLog, Log, TEXT("KEY: %s"), *Key);
		    UE_LOG(GameServerLog, Log, TEXT("VALUE: %s"), *Value);
			if (Key.Equals("extport"))
			{
				UE_LOG(GameServerLog, Log, TEXT("EXTPORT EXIST"));
				ProcessParameters->port = FCString::Atoi(*Value);
			}
		}
	}
	if (UNetDriver* Driver = GetWorld()->GetNetDriver())
	{
		TSharedPtr<const FInternetAddr> LocalAddr = Driver->GetLocalAddr();
		
		if (LocalAddr.IsValid())
		{
			UE_LOG(GameServerLog, Log, TEXT("PORT %i!"),LocalAddr->GetPort());
		}
	}

	TArray<FString> LogFiles;
	LogFiles.Add(TEXT("OdinFleet/Saved/Logs/server.log"));
	ProcessParameters->logParameters = LogFiles;

	UE_LOG(GameServerLog, Log, TEXT("Calling Process Ready..."));

	FGameLiftGenericOutcome ProcessReadyOutcome = GameLiftServerSdkModule->ProcessReady(*ProcessParameters);

	if (ProcessReadyOutcome.IsSuccess())
	{
		UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_GREEN);
		UE_LOG(GameServerLog, Log, TEXT("Process Ready!"));
		UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_NONE);
	}
	else
	{
		UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_RED);
		UE_LOG(GameServerLog, Log, TEXT("ERROR: Process Ready Failed!"));
		FGameLiftError ProcessReadyError = ProcessReadyOutcome.GetError();
		UE_LOG(GameServerLog, Log, TEXT("ERROR: %s"), *ProcessReadyError.m_errorMessage);
		UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_NONE);
	}
	UE_LOG(GameServerLog, Log, TEXT("InitGameLift completed!"));
	#endif
}


