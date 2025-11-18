// Fill out your copyright notice in the Description page of Project Settings.


#include "GLBSServiceConnector.h"

#include "GameLiftExeptions.h"


void UGLBSServiceConnector::GetSessions(FSearchComplete OnReady)
{	
	TFunction<void(const FJsonObject& Result, const FString& Error)> Done;
	FHttpModule& Module= FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Module.CreateRequest();

	Request->SetURL("https://europe-west3-odinfleettest.cloudfunctions.net/GameLiftSearchSessions");
	Request->SetVerb("GET");
	Request->SetHeader("Content-Type", "application/json");

	Request->OnProcessRequestComplete().BindLambda([OnReady](FHttpRequestPtr, FHttpResponsePtr Response,bool bOK)
	{
		const FString ResponseString = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
		TSharedPtr<FJsonObject> Json;
		if (!FJsonSerializer::Deserialize(Reader,Json) || !Json.IsValid())
		{
			TArray<FGameSessionData> GameSessions;
			FGameSessionData data;
			GameSessions.Add(data);
			OnReady.Execute(GameSessions,false,EGameLiftExceptionsBP::None);
			return;
		}
		if (Json->HasField(FString(TEXT("GameSessions"))))
		{
			TArray<FGameSessionData> GameSessionsStruct;
			FGameSessionData data;
			TArray<TSharedPtr<FJsonValue>> GameSessionsJson = Json->GetArrayField(FString(TEXT("GameSessions")));
			for (TSharedPtr<FJsonValue> GameSession : GameSessionsJson)
			{
				data = CreateGameSessionFromJson(GameSession);
				GameSessionsStruct.Add(data);
			}			
			OnReady.Execute(GameSessionsStruct,false,EGameLiftExceptionsBP::None);
			return;
		}
		TArray<FGameSessionData> GameSessions;
		FGameSessionData data;
		GameSessions.Add(data);
		OnReady.Execute(GameSessions,false,EGameLiftExceptionsBP::None);

	});

	Request->ProcessRequest();
}

void UGLBSServiceConnector::CreateGameSession(FSingleGameSessionResult OnCreated,FString CreatorId,FString GameSessionName)
{
	TFunction<void(const FJsonObject& Result, const FString& Error)> Done;
	FHttpModule& Module= FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Module.CreateRequest();

	Request->SetURL("https://europe-west3-odinfleettest.cloudfunctions.net/GameLiftCreateGameSession");
	
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");

	TSharedPtr<FJsonObject> JsonData = MakeShared<FJsonObject>();
	JsonData->SetStringField(TEXT("CreatorId"),CreatorId);
	JsonData->SetStringField(TEXT("SessionName"),GameSessionName);
	

	// Convert to string
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonData.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);
	
	Request->OnProcessRequestComplete().BindLambda([OnCreated](FHttpRequestPtr, FHttpResponsePtr Response,bool bOK)
	{
		const FString ResponseString = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
		TSharedPtr<FJsonObject> Json;
		EGameLiftExceptions c = static_cast<EGameLiftExceptions>(Response->GetResponseCode());
		if (c == FLEET_CAPACITY_EXCEPTION)
		{	FGameSessionData data;
			OnCreated.Execute(data,true,EGameLiftExceptionsBP::Fleet_capacity_Exception);
			return;
		}
		
		if (!FJsonSerializer::Deserialize(Reader,Json) || !Json.IsValid())
		{
			FGameSessionData data;
			OnCreated.Execute(data,true,EGameLiftExceptionsBP::Exception);
			return;
		}
		if (Json->HasField(FString(TEXT("GameSession"))))
		{
			FGameSessionData data;
			TSharedPtr<FJsonValue> GameSessionsJson = Json->GetField(FString(TEXT("GameSession")),EJson::Object);
			data = CreateGameSessionFromJson(GameSessionsJson);
			OnCreated.Execute(data,false,EGameLiftExceptionsBP::None);
			return;
		}
		FGameSessionData data;
		OnCreated.Execute(data,false,EGameLiftExceptionsBP::None);
	});
	Request->ProcessRequest();
}

void UGLBSServiceConnector::CloseGameSession(FSingleGameSessionResult OnClosed,FString GameSessionId)
{
	TFunction<void(const FJsonObject& Result, const FString& Error)> Done;
	FHttpModule& Module= FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Module.CreateRequest();

	Request->SetURL("https://europe-west3-odinfleettest.cloudfunctions.net/GameLiftCloseGameSession");
	Request->SetVerb("POST");
	Request->SetHeader("Content-Type", "application/json");

	
	TSharedPtr<FJsonObject> JsonData = MakeShared<FJsonObject>();
	JsonData->SetStringField(TEXT("GameSessionId"),GameSessionId);
	

	// Convert to string
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonData.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);
	
	Request->OnProcessRequestComplete().BindLambda([OnClosed](FHttpRequestPtr, FHttpResponsePtr Response,bool bOK)
	{
		const FString ResponseString = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
		TSharedPtr<FJsonObject> Json;
		if (!FJsonSerializer::Deserialize(Reader,Json) || !Json.IsValid())
		{
			FGameSessionData data;
			OnClosed.Execute(data,true,EGameLiftExceptionsBP::Exception);
			return;
		}
		if (Json->HasField(FString(TEXT("GameSession"))))
		{
			FGameSessionData data;
			TSharedPtr<FJsonObject> GameSession = Json->GetObjectField(FString(TEXT("GameSession")));
			if (GameSession->HasField(FString(TEXT("GameSession"))))
			{
				TSharedPtr<FJsonValue> GameSessionsJson = GameSession->GetField(FString(TEXT("GameSessions")),EJson::Object);
				data = CreateGameSessionFromJson(GameSessionsJson);
			}
			OnClosed.Execute(data,false,EGameLiftExceptionsBP::None);
			return;
		}
		FGameSessionData data;
		OnClosed.Execute(data,false,EGameLiftExceptionsBP::None);
	});
	Request->ProcessRequest();
}

FGameSessionData UGLBSServiceConnector::CreateGameSessionFromJson(TSharedPtr<FJsonValue> GameSessionJson)
{
	FGameSessionData data;
	TSharedPtr<FJsonObject> obj = GameSessionJson->AsObject();
	if (obj->HasField(TEXT("GameSessionId")))
	{
		data.GameSessionId =obj->GetStringField(TEXT("GameSessionId"));
	}
	if (obj->HasField(TEXT("Name")))
	{
		data.GameSessionName =obj->GetStringField(TEXT("Name"));
	}
	if (obj->HasField(TEXT("FleetId")))
	{
		data.FleetId =obj->GetStringField(TEXT("FleetId"));
	}
	if (obj->HasField(TEXT("CreationTime")))
	{
		FString CreationTime= obj->GetStringField(TEXT("CreationTime"));
		FDateTime Time;
		FDateTime::ParseIso8601(*obj->GetStringField(TEXT("CreationTime")),Time);
		data.CreationTime =Time;
	}
	if (obj->HasField(TEXT("TerminationTime")))
	{
		FString CreationTime= obj->GetStringField(TEXT("TerminationTime"));
		FDateTime Time;
		FDateTime::ParseIso8601(*obj->GetStringField(TEXT("TerminationTime")),Time);
		data.TerminationTime =Time;
	}
	if (obj->HasField(TEXT("CurrentPlayerSessionCount")))
	{
		data.CurrentPlayerCount =obj->GetIntegerField(TEXT("CurrentPlayerSessionCount"));
	}
	if (obj->HasField(TEXT("MaximumPlayerSessionCount")))
	{
		data.MaxPlayerCount =obj->GetIntegerField(TEXT("MaximumPlayerSessionCount"));
	}
	if (obj->HasField(TEXT("Status")))
	{
		data.Status =obj->GetStringField(TEXT("Status"));
	}
	if (obj->HasField(TEXT("StatusReason")))
	{
		data.StatusReason =obj->GetStringField(TEXT("StatusReason"));
	}
	if (obj->HasField(TEXT("IpAddress")))
	{
		data.IpAddress =obj->GetStringField(TEXT("IpAddress"));
	}
	if (obj->HasField(TEXT("DnsName")))
	{
		data.DnsName =obj->GetStringField(TEXT("DnsName"));
	}
	if (obj->HasField(TEXT("Port")))
	{
		data.Port =obj->GetIntegerField(TEXT("Port"));
	}
	if (obj->HasField(TEXT("CreatorId")))
	{
		data.CreatorId =obj->GetStringField(TEXT("CreatorId"));
	}
	if (obj->HasField(TEXT("Location")))
	{
		data.Location =obj->GetStringField(TEXT("Location"));
	}
	return data;
}

FPlayerSessionData UGLBSServiceConnector::CreatePlayerSessionFromJson(TSharedPtr<FJsonValue> PlayerSessionJson)
{
	FPlayerSessionData data;
	TSharedPtr<FJsonObject> obj = PlayerSessionJson->AsObject();
	if (obj->HasField(TEXT("PlayerSessionId")))
	{
		data.PlayerSessionId =obj->GetStringField(TEXT("PlayerSessionId"));
	}
	if (obj->HasField(TEXT("PlayerId")))
	{
		data.PlayerId =obj->GetStringField(TEXT("PlayerId"));
	}
	if (obj->HasField(TEXT("GameSessionId")))
	{
		data.GameSessionId =obj->GetStringField(TEXT("GameSessionId"));
	}
	if (obj->HasField(TEXT("FleetId")))
	{
		data.FleetId =obj->GetStringField(TEXT("FleetId"));
	}
	if (obj->HasField(TEXT("FleetArn")))
	{
		data.FleetArn =obj->GetStringField(TEXT("FleetArn"));
	}
	if (obj->HasField(TEXT("CreationTime")))
	{
		data.CreationTime =obj->GetIntegerField(TEXT("CreationTime"));
	}
	if (obj->HasField(TEXT("TerminationTime")))
	{
		data.TerminationTime =obj->GetIntegerField(TEXT("TerminationTime"));
	}
	if (obj->HasField(TEXT("Status")))
	{
		data.Status =obj->GetStringField(TEXT("Status"));
	}
	if (obj->HasField(TEXT("IpAddress")))
	{
		data.IpAddress =obj->GetStringField(TEXT("IpAddress"));
	}
	if (obj->HasField(TEXT("DnsName")))
	{
		data.DnsName =obj->GetStringField(TEXT("DnsName"));
	}
	if (obj->HasField(TEXT("Port")))
	{
		data.Port =obj->GetIntegerField(TEXT("Port"));
	}
	if (obj->HasField(TEXT("PlayerData")))
	{
		data.PlayerData =obj->GetStringField(TEXT("PlayerData"));
	}
	
	return data;
}
