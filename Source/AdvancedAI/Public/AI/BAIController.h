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

public:
	ABAIController();

protected:	
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

public:
	//UFUNCTION(BlueprintCallable)
	AActor* GetTargetActor();
	void SetTargetActor(AActor* InTargetActor);

private:

	UFUNCTION()
	void OnSenseTarget(AActor* InTargetActor);

	UPROPERTY(Category = "AI", EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	FName TargetActorKeyName;

	UPROPERTY(Category = "AI", EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	FName OriginKeyName;

	UPROPERTY(Category = "AI", VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY(Category = "AI", EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	UBehaviorTree* BehaviorTree;

	UPROPERTY()
	TWeakObjectPtr<AActor> CachedSelfPawn;
};
