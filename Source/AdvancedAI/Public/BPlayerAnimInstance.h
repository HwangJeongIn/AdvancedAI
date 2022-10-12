// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BPlayerAnimInstance.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionAnimationStateChanged);

enum class EActionType : uint8;

/**
 * 
 */
UCLASS()
class ADVANCEDAI_API UBPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	void PlayMontage(EActionType PlayerActionType);
	void StopMontage(EActionType PlayerActionType);

	void MontageJumpToSection(EActionType PlayerActionType, int32 SectionIndex);
private:

	FName SectionIndexToName(EActionType PlayerActionType, int32 SectionIndex);
	UAnimMontage* GetMontageByActionType(EActionType PlayerActionType);

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
	FOnActionAnimationStateChanged OnPrimaryAttackHit;

};
