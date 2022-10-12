// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BAction.h"
#include "BAction_PrimaryAttack.generated.h"


/**
 * 
 */
UCLASS()
class ADVANCEDAI_API UBAction_PrimaryAttack : public UBAction
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(UBActionComponent* NewActionComp) override;

	virtual void Start(AActor* InstigatorActor) override;
	virtual void Stop(AActor* InstigatorActor) override;


private:

	void Clear();

	UFUNCTION()
	void OnPrimaryAttackHit();

	UFUNCTION()
	void OnNextPrimaryAttackCheck();

	int32 SectionIndex;
	int32 MaxSectionIndex;
};
