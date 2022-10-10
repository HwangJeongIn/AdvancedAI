// Fill out your copyright notice in the Description page of Project Settings.


#include "BPlayerAnimInstance.h"

void UBPlayerAnimInstance::PlayPrimaryAttackMontage()
{
    if (!PrimaryAttackMontage)
    {
        B_ASSERT_DEV(false, " 확인해주세요. ");
        return;
    }

    if (!Montage_IsPlaying(PrimaryAttackMontage))
    {
        B_LOG_DEV("Montage_Play")
        Montage_Play(PrimaryAttackMontage, 1.0f);
    }
}