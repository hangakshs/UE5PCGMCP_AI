// Copyright Epic Games, Inc. All Rights Reserved.

#include "MCPClient.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

AMCPClient::AMCPClient()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;  // 0.1초마다 Tick
}

void AMCPClient::BeginPlay()
{
	Super::BeginPlay();

	if (bDebugMode)
	{
		UE_LOG(LogTemp, Log, TEXT("MCP Client initialized. Server URL: %s"), *ServerURL);

		if (bUseFileCommunication)
		{
			UE_LOG(LogTemp, Warning, TEXT("=== File-Based Communication Mode ==="));
			UE_LOG(LogTemp, Warning, TEXT("📁 Command Dir: %s"), *GetProjectIntermediatePath());
			UE_LOG(LogTemp, Warning, TEXT("✅ File Watcher Service auto-started via Python"));
			UE_LOG(LogTemp, Warning, TEXT("   (Started by Content/Python/init_unreal.py)"));
			UE_LOG(LogTemp, Warning, TEXT("====================================="));
		}
	}
}

void AMCPClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bUseFileCommunication)
	{
		return;
	}

	TimeSinceLastPoll += DeltaTime;

	if (TimeSinceLastPoll >= FilePollingInterval)
	{
		TimeSinceLastPoll = 0.0f;
		CheckCommandFile();
	}
}

void AMCPClient::SendCommand(const FString& Command)
{
	if (Command.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Empty command, ignoring"));
		return;
	}

	// 파일 기반 통신 사용
	if (bUseFileCommunication)
	{
		SendCommandViaFile(Command);
		return;
	}

	// HTTP 요청 생성
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &AMCPClient::HandleHttpResponse);

	// MCP 프로토콜에 맞는 JSON 페이로드 생성
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("jsonrpc"), TEXT("2.0"));
	JsonObject->SetNumberField(TEXT("id"), 1);
	JsonObject->SetStringField(TEXT("method"), TEXT("tools/call"));

	// 파라미터 객체
	TSharedPtr<FJsonObject> ParamsObject = MakeShareable(new FJsonObject);
	ParamsObject->SetStringField(TEXT("name"), TEXT("create_forest"));

	TSharedPtr<FJsonObject> ArgumentsObject = MakeShareable(new FJsonObject);
	ArgumentsObject->SetStringField(TEXT("command"), Command);

	ParamsObject->SetObjectField(TEXT("arguments"), ArgumentsObject);
	JsonObject->SetObjectField(TEXT("params"), ParamsObject);

	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	if (bDebugMode)
	{
		UE_LOG(LogTemp, Log, TEXT("Sending command to MCP: %s"), *Command);
		UE_LOG(LogTemp, Log, TEXT("Request body: %s"), *RequestBody);
	}

	// 실제 MCP 서버로 HTTP 요청 전송
	Request->SetURL(ServerURL + TEXT("/rpc"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(RequestBody);

	bool bRequestSent = Request->ProcessRequest();

	if (!bRequestSent)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to send HTTP request to MCP server"));
		OnMCPResponse.Broadcast(FString::Printf(TEXT("❌ MCP 서버 연결 실패: %s"), *ServerURL));
	}
	else if (bDebugMode)
	{
		UE_LOG(LogTemp, Log, TEXT("✅ HTTP request sent to: %s/rpc"), *ServerURL);
	}
}

void AMCPClient::ClearForest()
{
	UE_LOG(LogTemp, Log, TEXT("Clear forest command sent"));
	OnMCPResponse.Broadcast(TEXT("숲 제거 명령 전송됨"));

	// 빈 파라미터로 델리게이트 호출하여 숲 제거 시그널
	FPCGForestParameters EmptyParams;
	EmptyParams.AreaSize = 0.0f;
	OnForestGenerated.Broadcast(EmptyParams);
}

void AMCPClient::HandleHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("HTTP request failed"));
		OnMCPResponse.Broadcast(TEXT("MCP 서버 연결 실패"));
		return;
	}

	FString ResponseStr = Response->GetContentAsString();

	if (bDebugMode)
	{
		UE_LOG(LogTemp, Log, TEXT("MCP Response: %s"), *ResponseStr);
	}

	// JSON 파싱
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		// MCP 응답에서 결과 추출
		if (JsonObject->HasField(TEXT("result")))
		{
			TSharedPtr<FJsonObject> ResultObject = JsonObject->GetObjectField(TEXT("result"));

			if (ResultObject.IsValid() && ResultObject->HasField(TEXT("content")))
			{
				TArray<TSharedPtr<FJsonValue>> ContentArray = ResultObject->GetArrayField(TEXT("content"));

				for (const auto& ContentItem : ContentArray)
				{
					TSharedPtr<FJsonObject> ContentObject = ContentItem->AsObject();
					if (ContentObject.IsValid() && ContentObject->GetStringField(TEXT("type")) == TEXT("text"))
					{
						FString TextContent = ContentObject->GetStringField(TEXT("text"));

						// JSON 커맨드 추출 및 처리
						ProcessForestCommand(TextContent);

						OnMCPResponse.Broadcast(TextContent);
					}
				}
			}
		}
	}
}

void AMCPClient::ProcessForestCommand(const FString& JsonResponse)
{
	// JSON에서 Unreal Engine Command 부분 추출
	TSharedPtr<FJsonObject> CommandObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonResponse);

	if (FJsonSerializer::Deserialize(Reader, CommandObject) && CommandObject.IsValid())
	{
		FString Action = CommandObject->GetStringField(TEXT("action"));

		if (Action == TEXT("create_forest"))
		{
			TSharedPtr<FJsonObject> ParamsObject = CommandObject->GetObjectField(TEXT("parameters"));
			FPCGForestParameters Params = ParseForestParameters(ParamsObject);

			// 델리게이트 호출
			OnForestGenerated.Broadcast(Params);

			if (bDebugMode)
			{
				UE_LOG(LogTemp, Log, TEXT("Forest parameters parsed: Type=%s, Density=%s, MinDist=%.1f"),
					*Params.TreeType, *Params.Density, Params.MinDistance);
			}
		}
		else if (Action == TEXT("clear_forest"))
		{
			ClearForest();
		}
	}
}

FPCGForestParameters AMCPClient::ParseForestParameters(TSharedPtr<FJsonObject> ParamsObject)
{
	FPCGForestParameters Params;

	if (!ParamsObject.IsValid())
	{
		return Params;
	}

	Params.TreeType = ParamsObject->GetStringField(TEXT("tree_type"));
	Params.Density = ParamsObject->GetStringField(TEXT("density"));
	Params.Size = ParamsObject->GetStringField(TEXT("size"));
	Params.AreaSize = ParamsObject->GetNumberField(TEXT("area_size"));
	Params.MinDistance = ParamsObject->GetNumberField(TEXT("min_distance"));
	Params.MaxDistance = ParamsObject->GetNumberField(TEXT("max_distance"));
	Params.Randomness = ParamsObject->GetNumberField(TEXT("randomness"));
	Params.ScaleMultiplier = ParamsObject->GetNumberField(TEXT("scale_multiplier"));

	return Params;
}

FString AMCPClient::GetProjectIntermediatePath() const
{
	return FPaths::ProjectIntermediateDir() / TEXT("MCP_Commands");
}

void AMCPClient::SendCommandViaFile(const FString& Command)
{
	FString CommandDir = GetProjectIntermediatePath();
	FString CommandFilePath = CommandDir / TEXT("ue5_command.json");

	// 디렉토리 생성
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*CommandDir))
	{
		PlatformFile.CreateDirectoryTree(*CommandDir);
	}

	// JSON 생성
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("command"), Command);
	JsonObject->SetNumberField(TEXT("timestamp"), FPlatformTime::Seconds());

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	// 파일 저장
	if (FFileHelper::SaveStringToFile(JsonString, *CommandFilePath))
	{
		if (bDebugMode)
		{
			UE_LOG(LogTemp, Log, TEXT("✅ Command sent via file: %s"), *CommandFilePath);
			UE_LOG(LogTemp, Log, TEXT("   Command: %s"), *Command);
		}

		OnMCPResponse.Broadcast(FString::Printf(TEXT("📁 명령 파일 저장됨: %s"), *Command));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to write command file: %s"), *CommandFilePath);
		OnMCPResponse.Broadcast(TEXT("❌ 명령 파일 저장 실패"));
	}
}

void AMCPClient::CheckCommandFile()
{
	FString CommandDir = GetProjectIntermediatePath();
	FString ResponseFilePath = CommandDir / TEXT("mcp_response.json");

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// 응답 파일이 존재하는지 확인
	if (!PlatformFile.FileExists(*ResponseFilePath))
	{
		// 30초마다 한 번씩 안내 메시지 (선택적)
		static double LastWarningTime = 0.0;
		double CurrentTime = FPlatformTime::Seconds();

		if (bDebugMode && (CurrentTime - LastWarningTime) > 30.0)
		{
			LastWarningTime = CurrentTime;
			UE_LOG(LogTemp, Log, TEXT("⏳ Waiting for File Watcher Service response..."));
			UE_LOG(LogTemp, Log, TEXT("   File Watcher Service should be auto-started via Python"));
			UE_LOG(LogTemp, Log, TEXT("   If no response, check Output Log for Python errors"));
		}

		return;
	}

	// 파일 내용 읽기
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *ResponseFilePath))
	{
		return;
	}

	// 중복 처리 방지
	uint32 FileHash = GetTypeHash(JsonString);
	FString FileHashStr = FString::Printf(TEXT("%u"), FileHash);

	if (LastProcessedCommandHash == FileHashStr)
	{
		return;
	}

	LastProcessedCommandHash = FileHashStr;

	if (bDebugMode)
	{
		UE_LOG(LogTemp, Log, TEXT("📥 Received MCP response from file"));
	}

	// JSON 파싱
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		// action 필드 확인
		if (JsonObject->HasField(TEXT("action")))
		{
			ProcessForestCommand(JsonString);
		}
	}

	// 처리 완료 후 파일 삭제
	PlatformFile.DeleteFile(*ResponseFilePath);
}
