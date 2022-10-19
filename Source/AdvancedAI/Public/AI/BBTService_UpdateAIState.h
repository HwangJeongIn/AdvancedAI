// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BBTService_UpdateAIState.generated.h"

/**
 * 
 */
UCLASS()
class ADVANCEDAI_API UBBTService_UpdateAIState : public UBTService
{
	GENERATED_BODY()
	
public:
	UBBTService_UpdateAIState();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;


public:
	/*UFUNCTION(BlueprintCallable)
	void SetMaxDistanceToChase(float InMaxDistanceToChase);*/


private:
	UPROPERTY(Category = "AI", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	FBlackboardKeySelector TargetActorKey;

	UPROPERTY(Category = "AI", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	FBlackboardKeySelector HealthRateKey;

	UPROPERTY(Category = "AI", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	FBlackboardKeySelector CanAttackKey;

	UPROPERTY(Category = "AI", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float MaxDistanceToChase;

	UPROPERTY(Category = "AI", EditAnywhere, meta = (AllowPrivateAccess = "true"))
	float MaxDistanceToAttack;


	/*UPROPERTY()
	float MaxDistanceSquaredToChase;*/
};
