// Fill out your copyright notice in the Description page of Project Settings.


#include "BAnimInstance.h"
#include "BAction.h"


void UBAnimInstance::PlayMontage(EActionType PlayerActionType)
{
    UAnimMontage* Montage = GetMontageByActionType(PlayerActionType);
    if (nullptr == Montage)
    {
        B_ASSERT_DEV(false, "�ش� ��Ÿ�ְ� �������� �ʽ��ϴ�.");
        return;
    }

    if (true == Montage_IsPlaying(Montage))
    {
        B_LOG_DEV("�̹� �������Դϴ�.");
        return;
    }

    Montage_Play(Montage, 1.0f);
}

void UBAnimInstance::StopMontage(EActionType PlayerActionType)
{
    UAnimMontage* Montage = GetMontageByActionType(PlayerActionType);
    if (nullptr == Montage)
    {
        B_ASSERT_DEV(false, "�ش� ��Ÿ�ְ� �������� �ʽ��ϴ�.");
        return;
    }

    if (false == Montage_IsPlaying(Montage))
    {
        B_LOG_DEV("�������� ���°� �ƴմϴ�.");
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
        B_ASSERT_DEV(false, "�ش� ��Ÿ�ְ� �������� �ʽ��ϴ�.");
        return;
    }

    if (false == Montage_IsPlaying(Montage))
    {
        B_LOG_DEV("�������� ���°� �ƴմϴ�.");
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
    B_ASSERT_DEV(false, "�������Դϴ�.");
    return nullptr;
}