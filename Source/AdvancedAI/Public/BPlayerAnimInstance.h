// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BAnimInstance.h"
#include "BPlayerAnimInstance.generated.h"


/**
 * 
 */
UCLASS()
class ADVANCEDAI_API UBPlayerAnimInstance : public UBAnimInstance
{
	GENERATED_BODY()

protected:
	virtual UAnimMontage* GetMontageByActionType(EActionType PlayerActionType) override;

private:
	
	UFUNCTION()
	void AnimNotify_NextPrimaryAttackCheck();

	UFUNCTION()
	void AnimNotify_PrimaryAttackHitCheck();

	UPROPERTY(Category = "AttackAction", EditDefaultsOnly, BlueprintReadOnly, Meta = (AllowPrivateAccess = true))
	UAnimMontage* PrimaryAttackMontage;

public:
	UPROPERTY(Category = "AttackAction", BlueprintAssignable)
	FOnActionAnimationStateChanged OnNextPrimaryAttackCheck;
	
	UPROPERTY(Category = "AttackAction", BlueprintAssignable)
	FOnActionAnimationStateChanged OnPrimaryAttackHitCheck;

};
