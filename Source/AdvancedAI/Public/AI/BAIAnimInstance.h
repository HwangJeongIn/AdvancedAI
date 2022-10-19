// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BAnimInstance.h"
#include "BAIAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class ADVANCEDAI_API UBAIAnimInstance : public UBAnimInstance
{
	GENERATED_BODY()
protected:
	virtual UAnimMontage* GetMontageByActionType(EActionType PlayerActionType) override;

private:

	UFUNCTION()
	void AnimNotify_NextAIPrimaryAttackCheck();

	UFUNCTION()
	void AnimNotify_AIPrimaryAttackHitCheck();

	UPROPERTY(Category = "AttackAction", EditDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess = true))
	UAnimMontage* AIPrimaryAttackMontage;

public:
	UPROPERTY(Category = "AttackAction", BlueprintAssignable)
	FOnActionAnimationStateChanged OnNextAIPrimaryAttackCheck;
	
	UPROPERTY(Category = "AttackAction", BlueprintAssignable)
	FOnActionAnimationStateChanged OnAIPrimaryAttackHitCheck;
};
