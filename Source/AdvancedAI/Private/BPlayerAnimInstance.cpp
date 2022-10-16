// Fill out your copyright notice in the Description page of Project Settings.


#include "BPlayerAnimInstance.h"
#include "BAction.h"
#include "BPlayer.h"


UAnimMontage* UBPlayerAnimInstance::GetMontageByActionType(EActionType PlayerActionType)
{
    switch (PlayerActionType)
    {
    case EActionType::PrimaryAttack:
        return PrimaryAttackMontage;
    case EActionType::SecondaryAttack:
        return nullptr;
    default:
        return nullptr;
    }
}

void UBPlayerAnimInstance::AnimNotify_NextPrimaryAttackCheck()
{
    OnNextPrimaryAttackCheck.Broadcast();
}

void UBPlayerAnimInstance::AnimNotify_PrimaryAttackHitCheck()
{
    OnPrimaryAttackHit.Broadcast();
}