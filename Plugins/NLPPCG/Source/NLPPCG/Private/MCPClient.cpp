// Copyright Epic Games, Inc. All Rights Reserved.

#include "MCPClient.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Containers/Ticker.h"
#include "EngineUtils.h"
#include "ForestPCGManager.h"

AMCPClient::AMCPClient()
{
	// Tick을 비활성화 - FTSTicker 사용으로 전환
	PrimaryActorTick.bCanEverTick = false;
}

AMCPClient::~AMCPClient()
{
	// 타이머 정리
	StopPollingTimer();
}

void AMCPClient::Initialize()
{
	// 중복 초기화 방지
	if (bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("   ⚠️  Already initialized, skipping..."));
		return;
	}
	bIsInitialized = true;

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("🔧 MCPClient::Initialize() called"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("   Server URL: %s"), *ServerURL);
	UE_LOG(LogTemp, Warning, TEXT("   bUseFileCommunication: %s"), bUseFileCommunication ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Warning, TEXT("   bDebugMode: %s"), bDebugMode ? TEXT("TRUE") : TEXT("FALSE"));
	UE_LOG(LogTemp, Warning, TEXT("   FilePollingInterval: %.2f seconds"), FilePollingInterval);
	UE_LOG(LogTemp, Warning, TEXT("   ✅ Using FTSTicker-based polling (Editor-safe)"));

	if (bUseFileCommunication)
	{
		FString CommandDir = GetProjectIntermediatePath();
		UE_LOG(LogTemp, Warning, TEXT("   Target Command Dir: %s"), *CommandDir);

		// 디렉토리 미리 생성
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		UE_LOG(LogTemp, Warning, TEXT("   Checking directory existence..."));
		if (!PlatformFile.DirectoryExists(*CommandDir))
		{
			UE_LOG(LogTemp, Warning, TEXT("   ❌ Directory does not exist, creating..."));
			if (PlatformFile.CreateDirectoryTree(*CommandDir))
			{
				UE_LOG(LogTemp, Warning, TEXT("   ✅ Successfully created directory!"));

				// 생성 확인
				if (PlatformFile.DirectoryExists(*CommandDir))
				{
					UE_LOG(LogTemp, Warning, TEXT("   ✅ Directory existence verified!"));
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("   ❌ CRITICAL: Directory created but verification failed!"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("   ❌ CRITICAL: Failed to create directory!"));
				UE_LOG(LogTemp, Error, TEXT("   Forest generation will NOT work!"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("   ✅ Directory already exists"));
		}

		// 파일 목록 확인
		TArray<FString> FoundFiles;
		PlatformFile.FindFiles(FoundFiles, *CommandDir, nullptr);
		UE_LOG(LogTemp, Warning, TEXT("   Found %d files in directory"), FoundFiles.Num());

		if (FoundFiles.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("   Existing files:"));
			for (const FString& File : FoundFiles)
			{
				UE_LOG(LogTemp, Log, TEXT("      - %s"), *File);
			}
		}

		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("✅ File-Based Communication Mode Active"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("   Command Dir: %s"), *CommandDir);
		UE_LOG(LogTemp, Warning, TEXT("   File Watcher: Should be auto-started via Python"));
		UE_LOG(LogTemp, Warning, TEXT("   Polling Interval: %.2f seconds"), FilePollingInterval);
		UE_LOG(LogTemp, Warning, TEXT("========================================"));

		// FTSTicker 폴링 시작
		StartPollingTimer();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("⚠️  HTTP Communication Mode (File mode disabled)"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
	}
}

void AMCPClient::BeginPlay()
{
	Super::BeginPlay();

	// BeginPlay가 호출되면 Initialize 실행
	Initialize();
}

void AMCPClient::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// 에디터 환경에서 자동 초기화
	#if WITH_EDITOR
	if (!bIsInitialized)
	{
		UE_LOG(LogTemp, Warning, TEXT("🔧 PostInitializeComponents() - Auto-initializing in Editor mode"));
		Initialize();
	}
	#endif
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
	UE_LOG(LogTemp, Warning, TEXT("🔄 Processing Forest Command..."));

	// JSON에서 Unreal Engine Command 부분 추출
	TSharedPtr<FJsonObject> CommandObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonResponse);

	if (FJsonSerializer::Deserialize(Reader, CommandObject) && CommandObject.IsValid())
	{
		FString Action = CommandObject->GetStringField(TEXT("action"));
		UE_LOG(LogTemp, Warning, TEXT("   Action: %s"), *Action);

	if (Action == TEXT("create_forest") || Action == TEXT("modify_forest"))
		{
		const bool bIsModify = (Action == TEXT("modify_forest"));
			TSharedPtr<FJsonObject> ParamsObject = CommandObject->GetObjectField(TEXT("parameters"));
			FPCGForestParameters Params = ParseForestParameters(ParamsObject);

			UE_LOG(LogTemp, Warning, TEXT("   Parsed Parameters:"));
			UE_LOG(LogTemp, Warning, TEXT("      Tree Type: %s"), *Params.TreeType);
			UE_LOG(LogTemp, Warning, TEXT("      Density: %s"), *Params.Density);
			UE_LOG(LogTemp, Warning, TEXT("      Area Size: %.1f cm²"), Params.AreaSize);
			UE_LOG(LogTemp, Warning, TEXT("      Min Distance: %.1f cm"), Params.MinDistance);

			// 델리게이트 바인딩 개수 확인
			int32 BindingCount = OnForestGenerated.IsBound() ? 1 : 0;
			UE_LOG(LogTemp, Warning, TEXT("   OnForestGenerated delegate bound count: %d"), BindingCount);
			UE_LOG(LogTemp, Warning, TEXT("   OnForestGenerated IsBound: %s"), OnForestGenerated.IsBound() ? TEXT("YES") : TEXT("NO"));

			auto GenerateDirectly = [&]()
			{
				UE_LOG(LogTemp, Error, TEXT(">>> WORKAROUND: Finding ForestPCGManager directly..."));

				UWorld* World = GetWorld();
				if (World)
				{
					for (TActorIterator<AForestPCGManager> It(World); It; ++It)
					{
						AForestPCGManager* Manager = *It;
						if (Manager)
						{
							UE_LOG(LogTemp, Error, TEXT(">>> Found ForestPCGManager: %s <<<"), *Manager->GetName());
							UE_LOG(LogTemp, Error, TEXT(">>> Calling GenerateForestFromParameters DIRECTLY <<<"));
							Manager->GenerateForestFromParameters(Params);
							UE_LOG(LogTemp, Error, TEXT(">>> Direct call completed <<<"));
							break;
						}
					}
				}
			};

			if (!OnForestGenerated.IsBound())
			{
				UE_LOG(LogTemp, Error, TEXT("❌ CRITICAL: No listeners bound to OnForestGenerated!"));
				UE_LOG(LogTemp, Error, TEXT("   Make sure ForestPCGManager is in the level and has bound the delegate!"));
				GenerateDirectly();
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("   Broadcasting to ForestPCGManager..."));
				UE_LOG(LogTemp, Warning, TEXT("   Parameters being sent (%s):"), bIsModify ? TEXT("modify") : TEXT("create"));
				UE_LOG(LogTemp, Warning, TEXT("      - TreeType: '%s'"), *Params.TreeType);
				UE_LOG(LogTemp, Warning, TEXT("      - Density: '%s'"), *Params.Density);
				UE_LOG(LogTemp, Warning, TEXT("      - AreaSize: %.1f"), Params.AreaSize);

				// 델리게이트 호출
				UE_LOG(LogTemp, Error, TEXT(">>> ABOUT TO CALL OnForestGenerated.Broadcast() <<<"));
				OnForestGenerated.Broadcast(Params);
				UE_LOG(LogTemp, Error, TEXT(">>> OnForestGenerated.Broadcast() RETURNED <<<"));
				UE_LOG(LogTemp, Warning, TEXT("✅ Forest generation command broadcasted"));
			}
		}
		else if (Action == TEXT("clear_forest"))
		{
			UE_LOG(LogTemp, Warning, TEXT("   Clearing forest..."));
			ClearForest();
		}
		else if (Action == TEXT("error"))
		{
			FString ErrorMsg = CommandObject->GetStringField(TEXT("error"));
			UE_LOG(LogTemp, Error, TEXT("❌ Error from MCP: %s"), *ErrorMsg);
			OnMCPResponse.Broadcast(FString::Printf(TEXT("❌ 오류: %s"), *ErrorMsg));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("⚠️ Unknown action: %s"), *Action);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to deserialize forest command JSON"));
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

	// 밀도 배율 파싱 (선택적 필드)
	if (ParamsObject->HasField(TEXT("density_multiplier")))
	{
		Params.DensityMultiplier = ParamsObject->GetNumberField(TEXT("density_multiplier"));
	}

	// 여러 나무 타입 파싱 (tree_types 배열)
	if (ParamsObject->HasField(TEXT("tree_types")))
	{
		const TArray<TSharedPtr<FJsonValue>>* TreeTypesArray;
		if (ParamsObject->TryGetArrayField(TEXT("tree_types"), TreeTypesArray))
		{
			Params.TreeTypes.Empty();
			for (const TSharedPtr<FJsonValue>& Value : *TreeTypesArray)
			{
				FString TreeTypeStr;
				if (Value->TryGetString(TreeTypeStr))
				{
					Params.TreeTypes.Add(TreeTypeStr);
				}
			}
			UE_LOG(LogTemp, Warning, TEXT("   Parsed %d tree types for mixed forest"), Params.TreeTypes.Num());
		}
	}

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

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("🚀 Sending Command via File"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("   Command: %s"), *Command);
	UE_LOG(LogTemp, Warning, TEXT("   Target File: %s"), *CommandFilePath);

	// 디렉토리 생성
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*CommandDir))
	{
		UE_LOG(LogTemp, Warning, TEXT("   Creating directory: %s"), *CommandDir);
		if (!PlatformFile.CreateDirectoryTree(*CommandDir))
		{
			UE_LOG(LogTemp, Error, TEXT("❌ Failed to create directory!"));
			OnMCPResponse.Broadcast(TEXT("❌ 디렉토리 생성 실패"));
			return;
		}
	}

	// JSON 생성
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(TEXT("command"), Command);
	JsonObject->SetNumberField(TEXT("timestamp"), FPlatformTime::Seconds());

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	UE_LOG(LogTemp, Log, TEXT("   JSON Content: %s"), *JsonString);

	// 파일 저장 - UTF-8 인코딩 명시적 사용 (Python 호환성)
	if (FFileHelper::SaveStringToFile(JsonString, *CommandFilePath, FFileHelper::EEncodingOptions::ForceUTF8))
	{
		UE_LOG(LogTemp, Warning, TEXT("✅ Command file created successfully!"));
		UE_LOG(LogTemp, Warning, TEXT("   File size: %d bytes"), JsonString.Len());
		UE_LOG(LogTemp, Warning, TEXT("   Encoding: UTF-8 (for Python compatibility)"));
		UE_LOG(LogTemp, Warning, TEXT("   Waiting for File Watcher Service to process..."));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));

		OnMCPResponse.Broadcast(FString::Printf(TEXT("📁 명령 전송됨: %s"), *Command));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to write command file: %s"), *CommandFilePath);
		UE_LOG(LogTemp, Error, TEXT("========================================"));
		OnMCPResponse.Broadcast(TEXT("❌ 명령 파일 저장 실패"));
	}
}

void AMCPClient::StartPollingTimer()
{
	// 기존 타이머가 있다면 제거
	StopPollingTimer();

	UE_LOG(LogTemp, Warning, TEXT("⏰ Starting FTSTicker polling timer..."));
	UE_LOG(LogTemp, Warning, TEXT("   Interval: %.2f seconds"), FilePollingInterval);

	// FTSTicker 등록 - FilePollingInterval마다 TickerCallback 호출
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &AMCPClient::TickerCallback),
		FilePollingInterval
	);

	if (TickerHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("✅ FTSTicker polling timer started successfully!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to start FTSTicker polling timer!"));
	}
}

void AMCPClient::StopPollingTimer()
{
	if (TickerHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("🛑 Stopping FTSTicker polling timer..."));
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
		UE_LOG(LogTemp, Warning, TEXT("✅ Polling timer stopped"));
	}
}

bool AMCPClient::TickerCallback(float DeltaTime)
{
	// 첫 Ticker 콜백 로그 (한 번만)
	static bool bFirstCallback = true;
	if (bFirstCallback)
	{
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("🔄 FTSTicker::TickerCallback() - First callback!"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("   This confirms timer is working in Editor"));
		UE_LOG(LogTemp, Warning, TEXT("   DeltaTime: %.3f seconds"), DeltaTime);
		UE_LOG(LogTemp, Warning, TEXT("   Polling for response files..."));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		bFirstCallback = false;
	}

	// 파일 체크 실행
	CheckCommandFile();

	// true를 반환하면 타이머 계속 실행
	return true;
}

void AMCPClient::CheckCommandFile()
{
	FString CommandDir = GetProjectIntermediatePath();
	FString ResponseFilePath = CommandDir / TEXT("mcp_response.json");

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// 첫 체크 로그 (한 번만)
	static bool bFirstCheck = true;
	if (bFirstCheck)
	{
		UE_LOG(LogTemp, Warning, TEXT("📂 CheckCommandFile() - First check!"));
		UE_LOG(LogTemp, Warning, TEXT("   Command Dir: %s"), *CommandDir);
		UE_LOG(LogTemp, Warning, TEXT("   Response File Path: %s"), *ResponseFilePath);
		UE_LOG(LogTemp, Warning, TEXT("   Directory exists: %s"), PlatformFile.DirectoryExists(*CommandDir) ? TEXT("YES") : TEXT("NO"));
		bFirstCheck = false;
	}

	// 디렉토리 존재 확인
	if (!PlatformFile.DirectoryExists(*CommandDir))
	{
		static double LastDirWarningTime = 0.0;
		double CurrentTime = FPlatformTime::Seconds();

		if ((CurrentTime - LastDirWarningTime) > 30.0)
		{
			LastDirWarningTime = CurrentTime;
			UE_LOG(LogTemp, Error, TEXT("❌ CRITICAL: MCP_Commands directory does not exist!"));
			UE_LOG(LogTemp, Error, TEXT("   Expected path: %s"), *CommandDir);
			UE_LOG(LogTemp, Error, TEXT("   Forest generation will NOT work!"));
		}
		return;
	}

	// 응답 파일이 존재하는지 확인
	if (!PlatformFile.FileExists(*ResponseFilePath))
	{
		// 30초마다 한 번씩 안내 메시지
		static double LastWarningTime = 0.0;
		double CurrentTime = FPlatformTime::Seconds();

		if ((CurrentTime - LastWarningTime) > 30.0)
		{
			LastWarningTime = CurrentTime;
			UE_LOG(LogTemp, Log, TEXT("⏳ Waiting for File Watcher Service response..."));
			UE_LOG(LogTemp, Log, TEXT("   Expected file: %s"), *ResponseFilePath);
			UE_LOG(LogTemp, Log, TEXT("   Directory exists: %s"), PlatformFile.DirectoryExists(*CommandDir) ? TEXT("YES") : TEXT("NO"));

			// 디렉토리 내 파일 목록 확인
			TArray<FString> FoundFiles;
			PlatformFile.FindFiles(FoundFiles, *CommandDir, nullptr);
			UE_LOG(LogTemp, Log, TEXT("   Files in directory: %d"), FoundFiles.Num());

			if (FoundFiles.Num() > 0)
			{
				UE_LOG(LogTemp, Log, TEXT("   Existing files:"));
				for (const FString& File : FoundFiles)
				{
					UE_LOG(LogTemp, Log, TEXT("      - %s"), *FPaths::GetCleanFilename(File));
				}
			}

			UE_LOG(LogTemp, Log, TEXT("   File Watcher Service should be auto-started via Python"));
			UE_LOG(LogTemp, Log, TEXT("   If no response, check Output Log for Python errors"));
		}

		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("📥 Response File Detected!"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("   File: %s"), *ResponseFilePath);

	// 파일 내용 읽기
	FString JsonString;
	if (!FFileHelper::LoadFileToString(JsonString, *ResponseFilePath))
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to read response file"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("   Content: %s"), *JsonString);

	// 중복 처리 방지
	uint32 FileHash = GetTypeHash(JsonString);
	FString FileHashStr = FString::Printf(TEXT("%u"), FileHash);

	if (LastProcessedCommandHash == FileHashStr)
	{
		UE_LOG(LogTemp, Log, TEXT("   ⚠️ Already processed (duplicate), skipping"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		return;
	}

	LastProcessedCommandHash = FileHashStr;

	// JSON 파싱
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		// action 필드 확인
		if (JsonObject->HasField(TEXT("action")))
		{
			FString Action = JsonObject->GetStringField(TEXT("action"));
			UE_LOG(LogTemp, Warning, TEXT("✅ Processing action: %s"), *Action);
			ProcessForestCommand(JsonString);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("❌ No 'action' field in response"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("❌ Failed to parse JSON response"));
	}

	UE_LOG(LogTemp, Warning, TEXT("========================================"));

	// 처리 완료 후 파일 삭제
	if (PlatformFile.DeleteFile(*ResponseFilePath))
	{
		UE_LOG(LogTemp, Log, TEXT("   Response file deleted"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("   Failed to delete response file"));
	}
}
