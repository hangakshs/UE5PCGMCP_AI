// Copyright Epic Games, Inc. All Rights Reserved.

#include "PCGForestGenerator.h"
#include "PCGContext.h"
#include "Data/PCGPointData.h"
#include "Data/PCGSpatialData.h"
#include "Helpers/PCGAsync.h"
#include "Helpers/PCGHelpers.h"
#include "Metadata/PCGMetadata.h"
#include "Metadata/PCGMetadataAttribute.h"
#include "UObject/SoftObjectPath.h"

UPCGForestGeneratorSettings::UPCGForestGeneratorSettings()
{
	// 기본값 설정
}

#if WITH_EDITOR
void UPCGForestGeneratorSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 무한 재귀 방지: 이미 PreloadMeshes 중이면 스킵
	if (bIsPreloadingMeshes)
	{
		return;
	}

	// TreeMeshMap이 변경되면 캐시 갱신
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UPCGForestGeneratorSettings, TreeMeshMap))
	{
		PreloadMeshes();
	}
}
#endif

void UPCGForestGeneratorSettings::PostLoad()
{
	Super::PostLoad();
	PreloadMeshes();
}

void UPCGForestGeneratorSettings::PreloadMeshes()
{
	// 게임 스레드에서만 실행 가능
	if (!IsInGameThread())
	{
		return;
	}

	// 무한 재귀 방지: 이미 PreloadMeshes 실행 중이면 스킵
	if (bIsPreloadingMeshes)
	{
		UE_LOG(LogTemp, Warning, TEXT("PreloadMeshes() already in progress, skipping to prevent infinite recursion"));
		return;
	}

	// 재귀 방지 플래그 설정
	bIsPreloadingMeshes = true;

	CachedMeshes.Empty();
	CachedMeshBounds.Empty();

	for (const auto& MeshPair : TreeMeshMap)
	{
		const FString& TreeType = MeshPair.Key;
		const TSoftObjectPtr<UStaticMesh>& MeshPtr = MeshPair.Value;

		// 동기 로드 (게임 스레드에서만 호출)
		UStaticMesh* LoadedMesh = MeshPtr.LoadSynchronous();
		if (LoadedMesh)
		{
			CachedMeshes.Add(TreeType, LoadedMesh);
			CachedMeshBounds.Add(TreeType, LoadedMesh->GetBoundingBox());
			UE_LOG(LogTemp, Log, TEXT("Preloaded mesh for tree type '%s': Bounds Min=%s, Max=%s"),
				*TreeType,
				*LoadedMesh->GetBoundingBox().Min.ToString(),
				*LoadedMesh->GetBoundingBox().Max.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Failed to preload mesh for tree type '%s'"), *TreeType);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Preloaded %d/%d tree meshes"), CachedMeshes.Num(), TreeMeshMap.Num());

	// 재귀 방지 플래그 해제
	bIsPreloadingMeshes = false;
}

TArray<FPCGPinProperties> UPCGForestGeneratorSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	// 입력 핀 없음 (순수 생성자)
	return PinProperties;
}

TArray<FPCGPinProperties> UPCGForestGeneratorSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> PinProperties;
	PinProperties.Emplace(PCGPinConstants::DefaultOutputLabel, EPCGDataType::Point);
	return PinProperties;
}

FPCGElementPtr UPCGForestGeneratorSettings::CreateElement() const
{
	return MakeShared<FPCGForestGeneratorElement>();
}

bool FPCGForestGeneratorElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGForestGeneratorElement::Execute);

	const UPCGForestGeneratorSettings* Settings = Context->GetInputSettings<UPCGForestGeneratorSettings>();
	check(Settings);

	const FPCGForestParameters& Params = Settings->ForestParameters;

	// 최대 영역 크기 제한 (90,000,000,000 cm² = 3000m x 3000m = 900 헥타르)
	const float MaxAreaSize = 90000000000.0f; // 90,000,000,000 cm² = 3km x 3km
	float ClampedAreaSize = Params.AreaSize;

	if (Params.AreaSize > MaxAreaSize)
	{
		ClampedAreaSize = MaxAreaSize;
		float RequestedSide = FMath::Sqrt(Params.AreaSize);
		float ClampedSide = FMath::Sqrt(MaxAreaSize);
		UE_LOG(LogTemp, Warning, TEXT("⚠️  Forest area size limited for performance:"));
		UE_LOG(LogTemp, Warning, TEXT("   Requested: %.0f cm² (%.1f m x %.1f m)"),
			Params.AreaSize, RequestedSide / 100.0f, RequestedSide / 100.0f);
		UE_LOG(LogTemp, Warning, TEXT("   Limited to: %.0f cm² (%.1f m x %.1f m)"),
			MaxAreaSize, ClampedSide / 100.0f, ClampedSide / 100.0f);
		UE_LOG(LogTemp, Warning, TEXT("   This prevents memory overflow and ensures stable performance"));
	}

	// 출력 데이터 생성
	UPCGPointData* PointData = NewObject<UPCGPointData>();
	TArray<FPCGPoint>& Points = PointData->GetMutablePoints();

	// 랜덤 스트림 초기화
	FRandomStream RandomStream(Settings->Seed);

	// 여러 나무 타입이 있는지 확인
	TArray<FString> AvailableTreeTypes;
	bool bUseMixedForest = false;

	if (Params.TreeTypes.Num() > 0)
	{
		AvailableTreeTypes = Params.TreeTypes;
		bUseMixedForest = true;
		UE_LOG(LogTemp, Log, TEXT("Using mixed forest with %d tree types"), AvailableTreeTypes.Num());
	}
	else
	{
		AvailableTreeTypes.Add(Params.TreeType);
	}

	// TreeType 메타데이터 속성 추가 (모든 경우에)
	// FindOrCreateAttribute는 속성을 생성하거나 찾아서 반환
	if (PointData->Metadata)
	{
		PointData->Metadata->FindOrCreateAttribute<FString>(
			FName(TEXT("TreeType")),
			AvailableTreeTypes.Num() > 0 ? AvailableTreeTypes[0] : TEXT("generic_tree"),
			true
		);

		// MeshPath 메타데이터 속성 추가 (FSoftObjectPath)
		PointData->Metadata->FindOrCreateAttribute<FSoftObjectPath>(
			FName(TEXT("MeshPath")),
			FSoftObjectPath(),
			true
		);
		UE_LOG(LogTemp, Log, TEXT("TreeType and MeshPath metadata attributes created"));
	}

	// 경계 박스 설정 (제한된 영역 크기 사용)
	float SideLength = FMath::Sqrt(ClampedAreaSize);
	FVector BoundsMin = FVector(-SideLength * 0.5f, -SideLength * 0.5f, 0.0f);
	FVector BoundsMax = FVector(SideLength * 0.5f, SideLength * 0.5f, 0.0f);

	UE_LOG(LogTemp, Log, TEXT("📐 Forest Generation Area: %.1f m x %.1f m (%.0f cm²)"),
		SideLength / 100.0f, SideLength / 100.0f, ClampedAreaSize);

	// 메시 바운드 기반 기본 최소 거리 계산
	// 여러 메시 중 평균 크기를 사용하여 기본 간격 결정
	float AverageMeshSize = 0.0f;
	int32 ValidMeshCount = 0;
	for (const auto& MeshPair : Settings->CachedMeshBounds)
	{
		const FBox& Bounds = MeshPair.Value;
		// 2D 평면에서의 최대 반경 계산 (X, Y 중 큰 값)
		float MeshRadius = FMath::Max(
			FMath::Abs(Bounds.Min.X), FMath::Max(FMath::Abs(Bounds.Max.X),
			FMath::Max(FMath::Abs(Bounds.Min.Y), FMath::Abs(Bounds.Max.Y)))
		);
		AverageMeshSize += MeshRadius * 2.0f * Params.ScaleMultiplier; // 지름 계산
		ValidMeshCount++;
	}

	float BaseMeshSize = 200.0f; // 기본값 (메시가 없을 경우)
	if (ValidMeshCount > 0)
	{
		BaseMeshSize = AverageMeshSize / ValidMeshCount;
	}

	// 밀도 배율 적용 (수치 기반)
	// 밀도 1.0: 메시 바운드 크기만큼 배치, 바운드 겹침 없음
	// 밀도 2.0: 간격 = BaseMeshSize / sqrt(2), 바운드 겹침 허용
	// 밀도 0.5: 간격 = BaseMeshSize * sqrt(2), 더 드문드문 배치
	const float DensityMultiplier = FMath::Max(0.1f, Params.DensityMultiplier);

	// 밀도에 따른 기본 간격 계산
	// 면적당 나무 수 ∝ 1 / (간격²), 따라서 간격 ∝ 1 / sqrt(밀도)
	float DensityBasedMinDist = BaseMeshSize / FMath::Sqrt(DensityMultiplier);

	// 사용자가 MinDistance를 명시적으로 지정했는지 확인
	// 기본값(200.0f)이 아니면 사용자 지정 값 존재
	const bool bHasCustomMinDistance = !FMath::IsNearlyEqual(Params.MinDistance, 200.0f, 1.0f);

	// 최종 간격 결정
	// 사용자가 명시적으로 지정하지 않았으면 밀도 기반 계산 사용
	const float MinDist = bHasCustomMinDistance
		? Params.MinDistance
		: DensityBasedMinDist;
	const float MaxDist = FMath::Max(Params.MaxDistance, MinDist * 2.5f);

	const int32 MaxAttempts = 30;
	const float CellSize = MinDist / FMath::Sqrt(2.0f);

	// 밀도가 1.0 이상일 때 바운드 겹침 허용
	const bool bAllowBoundsOverlap = (DensityMultiplier > 1.0f);

	UE_LOG(LogTemp, Log, TEXT("🌲 Density-based spacing calculation:"));
	UE_LOG(LogTemp, Log, TEXT("   BaseMeshSize=%.1f cm, DensityMultiplier=%.2fx"), BaseMeshSize, DensityMultiplier);
	UE_LOG(LogTemp, Log, TEXT("   DensityBasedMinDist=%.1f cm (BaseMeshSize / sqrt(%.2f))"),
		DensityBasedMinDist, DensityMultiplier);
	UE_LOG(LogTemp, Log, TEXT("   Final MinDist=%.1f cm, MaxDist=%.1f cm"), MinDist, MaxDist);
	UE_LOG(LogTemp, Log, TEXT("   Allow Bounds Overlap: %s"), bAllowBoundsOverlap ? TEXT("YES") : TEXT("NO"));

	// 메모리 효율적인 해시맵 기반 그리드
	TArray<FVector> ActiveList;
	TMap<int64, TArray<int32>> SpatialHash; // 해시맵으로 변경 (메모리 효율적)

	// 첫 포인트 생성
	FVector FirstPoint = FVector(
		RandomStream.FRandRange(BoundsMin.X, BoundsMax.X),
		RandomStream.FRandRange(BoundsMin.Y, BoundsMax.Y),
		0.0f
	);
	ActiveList.Add(FirstPoint);

	// 해시 기반 그리드 인덱스 계산 (메모리 효율적)
	auto GetHashKey = [CellSize, BoundsMin](const FVector& Point) -> int64
	{
		int32 X = FMath::FloorToInt((Point.X - BoundsMin.X) / CellSize);
		int32 Y = FMath::FloorToInt((Point.Y - BoundsMin.Y) / CellSize);
		// 64비트 해시 키 생성 (X, Y를 32비트씩 사용)
		return (static_cast<int64>(X) << 32) | (static_cast<int64>(Y) & 0xFFFFFFFF);
	};

	// 해시 키에서 X, Y 좌표 추출
	auto GetGridCoords = [CellSize, BoundsMin](const FVector& Point, int32& OutX, int32& OutY)
	{
		OutX = FMath::FloorToInt((Point.X - BoundsMin.X) / CellSize);
		OutY = FMath::FloorToInt((Point.Y - BoundsMin.Y) / CellSize);
	};

	auto IsValidPoint = [&](const FVector& Point, const FBox& NewBounds, float Scale) -> bool
	{
		// 경계 체크
		if (Point.X < BoundsMin.X || Point.X > BoundsMax.X ||
			Point.Y < BoundsMin.Y || Point.Y > BoundsMax.Y)
		{
			return false;
		}

		// 현재 포인트의 그리드 좌표
		int32 GridX, GridY;
		GetGridCoords(Point, GridX, GridY);

		// 새 포인트의 스케일된 바운드 계산
		FBox ScaledNewBounds = NewBounds;
		ScaledNewBounds.Min *= Scale;
		ScaledNewBounds.Max *= Scale;

		// 주변 그리드 셀 확인 (해시맵 기반, 거리 및 바운드 기반 겹침 검사)
		// MinDist를 고려한 탐색 반경 (2셀 범위)
		for (int32 dx = -2; dx <= 2; ++dx)
		{
			for (int32 dy = -2; dy <= 2; ++dy)
			{
				int32 nx = GridX + dx;
				int32 ny = GridY + dy;

				// 해시 키 생성
				int64 HashKey = (static_cast<int64>(nx) << 32) | (static_cast<int64>(ny) & 0xFFFFFFFF);

				// 해시맵에서 해당 셀의 포인트 인덱스들 가져오기
				if (const TArray<int32>* PointIndices = SpatialHash.Find(HashKey))
				{
					for (int32 PointIdx : *PointIndices)
					{
						const FPCGPoint& ExistingPoint = Points[PointIdx];
						FVector ExistingLocation = ExistingPoint.Transform.GetLocation();

						// 기본 거리 체크 (항상 수행)
						float Distance = FVector::Distance(Point, ExistingLocation);
						if (Distance < MinDist)
						{
							return false;
						}

						// 바운드 기반 겹침 체크 (밀도가 1.0 이하일 때만)
						// 밀도가 1.0 이상이면 바운드 겹침 허용
						if (!bAllowBoundsOverlap)
						{
							FBox ExistingBounds = ExistingPoint.GetLocalBounds();
							FVector ExistingScale = FVector(ExistingPoint.Transform.GetScale3D());
							ExistingBounds.Min *= ExistingScale;
							ExistingBounds.Max *= ExistingScale;

							// 두 바운드를 월드 공간으로 변환하여 겹침 검사
							FBox WorldNewBounds = ScaledNewBounds.ShiftBy(Point);
							FBox WorldExistingBounds = ExistingBounds.ShiftBy(ExistingLocation);

							if (WorldNewBounds.Intersect(WorldExistingBounds))
							{
								return false; // 바운드가 겹침 (밀도 1.0 이하에서만)
							}
						}
					}
				}
			}
		}

		return true;
	};

	// 첫 포인트 추가
	FPCGPoint& FirstPCGPoint = Points.Add_GetRef(FPCGPoint());
	FirstPCGPoint.Transform = FTransform(FirstPoint);
	FirstPCGPoint.Density = 1.0f;

	// 첫 포인트에도 스케일 적용 (다른 포인트들과 동일하게)
	float FirstScaleVariation = RandomStream.FRandRange(0.8f, 1.2f);
	float FirstFinalScale = Params.ScaleMultiplier * FirstScaleVariation;
	FirstPCGPoint.Transform.SetScale3D(FVector(FirstFinalScale));

	// 첫 포인트에 TreeType 및 MeshPath 할당 (항상)
	FString SelectedTreeType;
	FBox MeshBounds(FVector(-50.0f), FVector(50.0f)); // 기본 바운드

	if (PointData->Metadata)
	{
		SelectedTreeType = AvailableTreeTypes[RandomStream.RandRange(0, AvailableTreeTypes.Num() - 1)];
		FirstPCGPoint.MetadataEntry = PointData->Metadata->AddEntry();

		// TreeType 어트리뷰트 설정
		FPCGMetadataAttribute<FString>* TreeTypeAttribute = PointData->Metadata->FindOrCreateAttribute<FString>(
			FName(TEXT("TreeType")),
			TEXT("generic_tree"),
			true
		);
		if (TreeTypeAttribute)
		{
			TreeTypeAttribute->SetValue(FirstPCGPoint.MetadataEntry, SelectedTreeType);
		}

		// MeshPath 어트리뷰트 설정 및 실제 메시 바운드 가져오기
		FPCGMetadataAttribute<FSoftObjectPath>* MeshPathAttribute = PointData->Metadata->FindOrCreateAttribute<FSoftObjectPath>(
			FName(TEXT("MeshPath")),
			FSoftObjectPath(),
			true
		);
		if (MeshPathAttribute && Settings->TreeMeshMap.Contains(SelectedTreeType))
		{
			TSoftObjectPtr<UStaticMesh> MeshPtr = Settings->TreeMeshMap[SelectedTreeType];
			MeshPathAttribute->SetValue(FirstPCGPoint.MetadataEntry, MeshPtr.ToSoftObjectPath());

			// 캐시된 바운드 사용 (스레드 안전)
			if (Settings->CachedMeshBounds.Contains(SelectedTreeType))
			{
				MeshBounds = Settings->CachedMeshBounds[SelectedTreeType];
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Mesh bounds not cached for tree type '%s', using default"), *SelectedTreeType);
			}
		}
	}

	// 실제 메시 바운드를 포인트에 설정
	FirstPCGPoint.SetLocalBounds(MeshBounds);

	// 랜덤 회전 (다른 포인트들과 동일하게)
	FRotator FirstRotation(0.0f, RandomStream.FRandRange(0.0f, 360.0f), 0.0f);
	FirstPCGPoint.Transform.SetRotation(FirstRotation.Quaternion());

	// 첫 포인트를 해시맵에 추가
	int64 FirstHashKey = GetHashKey(FirstPoint);
	SpatialHash.FindOrAdd(FirstHashKey).Add(0);

	// 전체 영역과 간격을 기반으로 최대 포인트 수 계산 (밀도 영향 반영)
	const float EffectiveCellArea = FMath::Max(KINDA_SMALL_NUMBER, MinDist * MinDist);
	const int32 DynamicPointLimit = FMath::Clamp(
		static_cast<int32>((ClampedAreaSize / EffectiveCellArea) * 1.5f),
		1000,
		200000);

	UE_LOG(LogTemp, Log, TEXT("   Target Point Count ≈ %d (limit)"), DynamicPointLimit);

	// Poisson Disk Sampling
	while (ActiveList.Num() > 0 && Points.Num() < DynamicPointLimit)
	{
		int32 RandomIndex = RandomStream.RandRange(0, ActiveList.Num() - 1);
		FVector Point = ActiveList[RandomIndex];

		bool bFoundValid = false;

		for (int32 Attempt = 0; Attempt < MaxAttempts; ++Attempt)
		{
			// 밀도 기반으로 계산된 MinDist와 MaxDist 사용
			float Angle = RandomStream.FRand() * 2.0f * PI;
			float Distance = RandomStream.FRandRange(MinDist, MaxDist);

			FVector NewPoint = Point + FVector(
				FMath::Cos(Angle) * Distance,
				FMath::Sin(Angle) * Distance,
				0.0f
			);

			// 스케일과 바운드를 미리 계산 (IsValidPoint에서 사용)
			float ScaleVariation = RandomStream.FRandRange(0.8f, 1.2f);
			float FinalScale = Params.ScaleMultiplier * ScaleVariation;

			// 나무 타입 선택
			FString NewSelectedTreeType = AvailableTreeTypes[RandomStream.RandRange(0, AvailableTreeTypes.Num() - 1)];
			FBox NewMeshBounds(FVector(-50.0f), FVector(50.0f)); // 기본 바운드

			// 캐시된 메시 바운드 사용 (스레드 안전)
			if (Settings->CachedMeshBounds.Contains(NewSelectedTreeType))
			{
				NewMeshBounds = Settings->CachedMeshBounds[NewSelectedTreeType];
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("Mesh bounds not cached for tree type '%s', using default"), *NewSelectedTreeType);
			}

			// 바운드와 스케일을 고려한 유효성 검사
			if (IsValidPoint(NewPoint, NewMeshBounds, FinalScale))
			{
				FPCGPoint& NewPCGPoint = Points.Add_GetRef(FPCGPoint());
				NewPCGPoint.Transform = FTransform(NewPoint);
				NewPCGPoint.Density = 1.0f;
				NewPCGPoint.Transform.SetScale3D(FVector(FinalScale));

				// TreeType 및 MeshPath 할당
				if (PointData->Metadata)
				{
					NewPCGPoint.MetadataEntry = PointData->Metadata->AddEntry();

					// TreeType 어트리뷰트 설정
					FPCGMetadataAttribute<FString>* TreeTypeAttribute = PointData->Metadata->FindOrCreateAttribute<FString>(
						FName(TEXT("TreeType")),
						TEXT("generic_tree"),
						true
					);
					if (TreeTypeAttribute)
					{
						TreeTypeAttribute->SetValue(NewPCGPoint.MetadataEntry, NewSelectedTreeType);
					}

					// MeshPath 어트리뷰트 설정
					FPCGMetadataAttribute<FSoftObjectPath>* MeshPathAttribute = PointData->Metadata->FindOrCreateAttribute<FSoftObjectPath>(
						FName(TEXT("MeshPath")),
						FSoftObjectPath(),
						true
					);
					if (MeshPathAttribute && Settings->TreeMeshMap.Contains(NewSelectedTreeType))
					{
						TSoftObjectPtr<UStaticMesh> MeshPtr = Settings->TreeMeshMap[NewSelectedTreeType];
						MeshPathAttribute->SetValue(NewPCGPoint.MetadataEntry, MeshPtr.ToSoftObjectPath());
					}
				}

				// 실제 메시 바운드를 포인트에 설정
				NewPCGPoint.SetLocalBounds(NewMeshBounds);

				// 랜덤 회전
				FRotator Rotation(0.0f, RandomStream.FRandRange(0.0f, 360.0f), 0.0f);
				NewPCGPoint.Transform.SetRotation(Rotation.Quaternion());

				ActiveList.Add(NewPoint);

				// 해시맵에 새 포인트 추가
				int64 NewHashKey = GetHashKey(NewPoint);
				SpatialHash.FindOrAdd(NewHashKey).Add(Points.Num() - 1);

				bFoundValid = true;
				break;
			}
		}

		if (!bFoundValid)
		{
			ActiveList.RemoveAt(RandomIndex);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Generated %d tree points"), Points.Num());

	// 출력 데이터 추가
	TArray<FPCGTaggedData>& Outputs = Context->OutputData.TaggedData;
	FPCGTaggedData& Output = Outputs.Emplace_GetRef();
	Output.Data = PointData;

	return true;
}
