// Fill out your copyright notice in the Description page of Project Settings.


#include "BAnimInstance.h"
#include "BAction.h"


void UBAnimInstance::PlayMontage(EActionType PlayerActionType)
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

void UBAnimInstance::StopMontage(EActionType PlayerActionType)
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

void UBAnimInstance::MontageJumpToSection(EActionType PlayerActionType, int32 SectionIndex)
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

FName UBAnimInstance::SectionIndexToName(EActionType PlayerActionType, int32 SectionIndex)
{
    return FName(*FString::Printf(TEXT("%d"), SectionIndex));
}

UAnimMontage* UBAnimInstance::GetMontageByActionType(EActionType PlayerActionType)
{
    B_ASSERT_DEV(false, "비정상입니다.");
    return nullptr;
}