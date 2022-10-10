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

	// ��������Ʈ �� ����
		
private:

	//UFUCNTION()
	//void AnimNotify_PrimaryAttackHitCheck();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Attack, Meta = (AllowPrivateAccess = true))
	UAnimMontage* PrimaryAttackMontage;
};
