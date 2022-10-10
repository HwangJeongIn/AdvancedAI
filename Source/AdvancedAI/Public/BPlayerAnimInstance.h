// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BPlayerAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class ADVANCEDAI_API UBPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	void PlayPrimaryAttackMontage();

	// 델리게이트 등 정의
		
private:

	//UFUCNTION()
	//void AnimNotify_PrimaryAttackHitCheck();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Attack, Meta = (AllowPrivateAccess = true))
	UAnimMontage* PrimaryAttackMontage;
};
