// Copyright Epic Games, Inc. All Rights Reserved.

#include "MCPClient.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"

AMCPClient::AMCPClient()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMCPClient::BeginPlay()
{
	Super::BeginPlay();

	if (bDebugMode)
	{
		UE_LOG(LogTemp, Log, TEXT("MCP Client initialized. Server URL: %s"), *ServerURL);
	}
}

void AMCPClient::SendCommand(const FString& Command)
{
	if (Command.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Empty command, ignoring"));
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

	// 실제 MCP 서버가 없을 경우를 대비한 시뮬레이션 모드
	// 실제 환경에서는 아래 주석을 해제하고 시뮬레이션 코드를 제거
	/*
	Request->SetURL(ServerURL + TEXT("/rpc"));
	Request->SetVerb(TEXT("POST"));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	Request->SetContentAsString(RequestBody);
	Request->ProcessRequest();
	*/

	// === 시뮬레이션 모드 (테스트용) ===
	// MCP 서버 응답 시뮬레이션
	FString SimulatedResponse = FString::Printf(TEXT(
		R"({
			"action": "create_forest",
			"parameters": {
				"tree_type": "pine",
				"density": "dense",
				"size": "medium",
				"area_size": 5000.0,
				"min_distance": 150.0,
				"max_distance": 300.0,
				"randomness": 0.3,
				"scale_multiplier": 1.0
			}
		})"
	));

	// 시뮬레이션 응답 처리
	ProcessForestCommand(SimulatedResponse);

	// 응답 브로드캐스트
	OnMCPResponse.Broadcast(FString::Printf(TEXT("시뮬레이션 모드: '%s' 명령 처리됨"), *Command));
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
