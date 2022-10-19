// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BBTTask_StartAction.generated.h"


enum class EActionType : uint8;

/**
 * 
 */
UCLASS()
class ADVANCEDAI_API UBBTTask_StartAction : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBBTTask_StartAction();

private:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

private:

	UPROPERTY(Category = "AI", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(Category = "AI", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	FBlackboardKeySelector CurrentActionTypeKey;

	UPROPERTY(Category = "AI", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	EActionType ActionType;
};
