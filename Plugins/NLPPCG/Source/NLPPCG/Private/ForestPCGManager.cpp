// Copyright Epic Games, Inc. All Rights Reserved.

#include "ForestPCGManager.h"
#include "PCGGraph.h"
#include "Elements/PCGStaticMeshSpawner.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AForestPCGManager::AForestPCGManager()
{
	PrimaryActorTick.bCanEverTick = false;

	// PCG 컴포넌트 생성
	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCGComponent"));
	RootComponent = PCGComponent;

	// 기본 메시 로드
	LoadDefaultTreeMesh();
}

void AForestPCGManager::BeginPlay()
{
	Super::BeginPlay();

	// MCP 클라이언트 자동 생성
	if (bAutoCreateMCPClient && !MCPClient)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		MCPClient = GetWorld()->SpawnActor<AMCPClient>(AMCPClient::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);

		if (MCPClient)
		{
			// 델리게이트 바인딩
			MCPClient->OnForestGenerated.AddDynamic(this, &AForestPCGManager::OnForestParametersReceived);
			UE_LOG(LogTemp, Log, TEXT("MCP Client created and bound to Forest Manager"));
		}
	}
	else if (MCPClient)
	{
		// 기존 클라이언트에 델리게이트 바인딩
		MCPClient->OnForestGenerated.AddDynamic(this, &AForestPCGManager::OnForestParametersReceived);
	}
}

void AForestPCGManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// 에디터에서 액터 배치 시 기본 설정
	if (!TreeMesh)
	{
		LoadDefaultTreeMesh();
	}
}

void AForestPCGManager::GenerateForestFromNLP(const FString& Command)
{
	if (!MCPClient)
	{
		UE_LOG(LogTemp, Error, TEXT("MCP Client not found!"));
		return;
	}

	MCPClient->SendCommand(Command);
}

void AForestPCGManager::ClearForest()
{
	if (PCGComponent)
	{
		PCGComponent->Cleanup();
		UE_LOG(LogTemp, Log, TEXT("Forest cleared"));
	}
}

void AForestPCGManager::GenerateForestFromParameters(const FPCGForestParameters& Parameters)
{
	SetupPCGGraph(Parameters);
}

void AForestPCGManager::OnForestParametersReceived(const FPCGForestParameters& Parameters)
{
	UE_LOG(LogTemp, Log, TEXT("Received forest parameters from MCP"));

	// 숲 제거 시그널 (AreaSize가 0)
	if (Parameters.AreaSize <= 0.0f)
	{
		ClearForest();
		return;
	}

	SetupPCGGraph(Parameters);
}

void AForestPCGManager::SetupPCGGraph(const FPCGForestParameters& Parameters)
{
	if (!PCGComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("PCG Component not found!"));
		return;
	}

	// PCG 그래프 생성 또는 가져오기
	UPCGGraph* PCGGraph = PCGComponent->GetGraph();
	if (!PCGGraph)
	{
		PCGGraph = NewObject<UPCGGraph>(PCGComponent);
		PCGComponent->SetGraph(PCGGraph);
	}

	// 기존 노드 제거
	PCGGraph->RemoveAllNodes();

	// 1. Forest Generator 노드 생성
	UPCGForestGeneratorSettings* ForestSettings = NewObject<UPCGForestGeneratorSettings>(PCGGraph);
	ForestSettings->ForestParameters = Parameters;

	// 영역 크기 계산 (AreaSize는 cm² 단위)
	float SideLength = FMath::Sqrt(Parameters.AreaSize);
	ForestSettings->BoundsSize = FVector(SideLength, SideLength, 1000.0f);

	UPCGNode* ForestNode = PCGGraph->AddNode(ForestSettings);

	// 2. Static Mesh Spawner 노드 생성
	UPCGStaticMeshSpawnerSettings* SpawnerSettings = NewObject<UPCGStaticMeshSpawnerSettings>(PCGGraph);

	// 메시 설정
	if (TreeMesh)
	{
		FPCGStaticMeshSpawnerEntry MeshEntry;
		MeshEntry.Weight = 100;
		MeshEntry.Mesh = TSoftObjectPtr<UStaticMesh>(TreeMesh);

		SpawnerSettings->Meshes.Add(MeshEntry);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Tree mesh not set, using default"));
	}

	// 스폰 옵션 설정
	SpawnerSettings->OutAttributeName = FName("TreeInstance");
	SpawnerSettings->bApplyMeshBoundsToPoints = true;

	UPCGNode* SpawnerNode = PCGGraph->AddNode(SpawnerSettings);

	// 노드 연결: Forest Generator -> Mesh Spawner
	ForestNode->GetOutputPin(PCGPinConstants::DefaultOutputLabel)->AddEdgeTo(
		SpawnerNode->GetInputPin(PCGPinConstants::DefaultInputLabel)
	);

	// 출력 노드 설정
	SpawnerNode->GetOutputPin(PCGPinConstants::DefaultOutputLabel)->AddEdgeTo(
		PCGGraph->GetOutputNode()->GetInputPin(PCGPinConstants::DefaultInputLabel)
	);

	// PCG 생성 실행
	PCGComponent->Generate();

	UE_LOG(LogTemp, Log, TEXT("PCG Forest generated with parameters: Density=%s, MinDist=%.1f"),
		*Parameters.Density, Parameters.MinDistance);
}

void AForestPCGManager::LoadDefaultTreeMesh()
{
	// 엔진 기본 큐브 메시 사용
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		TreeMesh = CubeMeshFinder.Object;
		UE_LOG(LogTemp, Log, TEXT("Default tree mesh (Cube) loaded"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to load default cube mesh"));
	}
}
