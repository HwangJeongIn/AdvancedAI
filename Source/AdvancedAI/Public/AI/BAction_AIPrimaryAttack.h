// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BAction.h"
#include "BAction_AIPrimaryAttack.generated.h"

/**
 * 
 */
UCLASS()
class ADVANCEDAI_API UBAction_AIPrimaryAttack : public UBAction
{
	GENERATED_BODY()
public:
	virtual void Initialize(UBActionComponent* NewActionComp) override;

	virtual void Start(AActor* InstigatorActor) override;
	virtual void Stop(AActor* InstigatorActor) override;

private:

	void Clear();

	UFUNCTION()
	void OnAIPrimaryAttackHitCheck();

	UFUNCTION()
	void OnNextAIPrimaryAttackCheck();

	int32 SectionIndex;

	UPROPERTY(Category = "AttackAction", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	int32 MaxSectionIndex = 5;

	UPROPERTY(Category = "AttackAction", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float SweepRadius = 20.0f;

	UPROPERTY(Category = "AttackAction", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float ActionRange = 20.0f;

	UPROPERTY(Category = "AttackAction", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float ActionOffset = 5.0f;

	UPROPERTY(Category = "AttackAction", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	float Damage = 50.0f;
};
