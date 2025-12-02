// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "ForestPCGManager.h"
#include "ForestEditorUtility.generated.h"

/**
 * 에디터 유틸리티 위젯 - 숲 생성 UI
 *
 * 사용법:
 * 1. Content Browser에서 우클릭 > Editor Utilities > Editor Utility Widget
 * 2. 부모 클래스를 UForestEditorUtility로 설정
 * 3. Designer에서 UI 디자인 (TextBox, Button 등)
 * 4. 버튼 OnClicked 이벤트에서 GenerateForestFromUI() 호출
 */
UCLASS(Blueprintable, BlueprintType)
class NLPPCG_API UForestEditorUtility : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	/**
	 * UI에서 입력받은 명령으로 숲 생성
	 * @param Command 자연어 명령
	 * @return 생성 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Forest")
	bool GenerateForestFromUI(const FString& Command);

	/**
	 * 모든 숲 제거
	 */
	UFUNCTION(BlueprintCallable, Category = "Forest")
	void ClearAllForests();

	/**
	 * 현재 레벨의 ForestPCGManager 개수
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Forest")
	int32 GetForestManagerCount() const;

	/**
	 * 마지막 생성 결과 메시지
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Forest")
	FString LastResultMessage;

	/**
	 * 예제 명령 목록
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Forest")
	TArray<FString> GetExampleCommands() const;
};
