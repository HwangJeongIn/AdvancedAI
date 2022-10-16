// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BAnimInstance.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnActionAnimationStateChanged);

enum class EActionType : uint8;

/**
 * 
 */
UCLASS()
class ADVANCEDAI_API UBAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	void PlayMontage(EActionType PlayerActionType);
	void StopMontage(EActionType PlayerActionType);

	void MontageJumpToSection(EActionType PlayerActionType, int32 SectionIndex);

private:
	FName SectionIndexToName(EActionType PlayerActionType, int32 SectionIndex);

protected:
	virtual UAnimMontage* GetMontageByActionType(EActionType PlayerActionType);
};
