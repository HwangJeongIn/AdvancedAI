// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BAIAnimInstance.h"
#include "BAction.h"

UAnimMontage* UBAIAnimInstance::GetMontageByActionType(EActionType PlayerActionType)
{
    switch (PlayerActionType)
    {
    case EActionType::AIPrimaryAttack:
        return AIPrimaryAttackMontage;
    default:
        return nullptr;
    }
}

void UBAIAnimInstance::AnimNotify_NextAIPrimaryAttackCheck()
{
    OnNextAIPrimaryAttackCheck.Broadcast();
}

void UBAIAnimInstance::AnimNotify_AIPrimaryAttackHitCheck()
{
    OnAIPrimaryAttackHitCheck.Broadcast();
}