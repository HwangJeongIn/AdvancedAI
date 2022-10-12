// Fill out your copyright notice in the Description page of Project Settings.


#include "BPlayerAnimInstance.h"
#include "BAction.h"


void UBPlayerAnimInstance::PlayMontage(EActionType PlayerActionType)
{
    UAnimMontage* Montage = GetMontageByActionType(PlayerActionType);
    if (nullptr == Montage)
    {
        B_ASSERT_DEV(false, "해당 몽타주가 존재하지 않습니다.");
        return;
    }

    if (true == Montage_IsPlaying(Montage))
    {
        B_LOG_DEV("이미 실행중입니다.");
        return;
    }

    Montage_Play(Montage, 1.0f);
}

void UBPlayerAnimInstance::StopMontage(EActionType PlayerActionType)
{
    UAnimMontage* Montage = GetMontageByActionType(PlayerActionType);
    if (nullptr == Montage)
    {
        B_ASSERT_DEV(false, "해당 몽타주가 존재하지 않습니다.");
        return;
    }

    if (false == Montage_IsPlaying(Montage))
    {
        B_LOG_DEV("실행중인 상태가 아닙니다.");
        return;
    }

    Montage_Stop(1.0f, Montage);
}

void UBPlayerAnimInstance::MontageJumpToSection(EActionType PlayerActionType, int32 SectionIndex)
{
    FName SectionName = SectionIndexToName(PlayerActionType, SectionIndex);
    UAnimMontage* Montage = GetMontageByActionType(PlayerActionType);
    if (nullptr == Montage)
    {
        B_ASSERT_DEV(false, "해당 몽타주가 존재하지 않습니다.");
        return;
    }

    if (false == Montage_IsPlaying(Montage))
    {
        B_LOG_DEV("실행중인 상태가 아닙니다.");
        return;
    }

    Montage_JumpToSection(SectionName, Montage);
}

FName UBPlayerAnimInstance::SectionIndexToName(EActionType PlayerActionType, int32 SectionIndex) 
{
    return FName(*FString::Printf(TEXT("%d"), SectionIndex));
}

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