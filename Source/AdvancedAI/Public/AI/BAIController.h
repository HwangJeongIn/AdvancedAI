// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "BAIController.generated.h"


class UBehaviorTree;

/**
 * 
 */
UCLASS()
class ADVANCEDAI_API ABAIController : public AAIController
{
	GENERATED_BODY()
protected:
	
	virtual void BeginPlay() override;

private:

	UPROPERTY(Category = "AI", EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	UBehaviorTree* BehaviorTree;
};
