// Fill out your copyright notice in the Description page of Project Settings.


#include "BAction_PrimaryAttack.h"
#include "BPlayer.h"
#include "BPlayerAnimInstance.h"
#include "BActionComponent.h"
#include "BStatusComponent.h"
#include "DrawDebugHelpers.h"


static int32 PrintPrimaryAttack = 1;
FAutoConsoleVariableRef CVARDebugPrintPrimaryAttack(
	TEXT("B.PrintPrimaryAttack"),
	PrintPrimaryAttack,
	TEXT("Print Primary Attack"),
	ECVF_Cheat);


void UBAction_PrimaryAttack::Initialize(UBActionComponent* NewActionComp)
{
	Super::Initialize(NewActionComp);

	ABPlayer* Player = Cast<ABPlayer>(NewActionComp->GetOwner());
	if (nullptr == Player)
	{
		B_ASSERT_DEV(false, "�÷��̾ �����ϴ�.");
		return;
	}

	UBPlayerAnimInstance* PlayerAnimInstance = Player->GetPlayerAnimInstance();
	PlayerAnimInstance->OnNextPrimaryAttackCheck.AddDynamic(this, &UBAction_PrimaryAttack::OnNextPrimaryAttackCheck);
	PlayerAnimInstance->OnPrimaryAttackHitCheck.AddDynamic(this, &UBAction_PrimaryAttack::OnPrimaryAttackHitCheck);

	Clear();
}

void UBAction_PrimaryAttack::Start(AActor* InstigatorActor)
{
	ABPlayer* Player = Cast<ABPlayer>(InstigatorActor);
	if (nullptr == Player)
	{
		B_ASSERT_DEV(false, "Player �� �����ϴ�.");
		return;
	}

	UBPlayerAnimInstance* PlayerAnimInstance = Player->GetPlayerAnimInstance();
	if (nullptr == PlayerAnimInstance)
	{
		B_ASSERT_DEV(false, "AnimInstance �� �����ϴ�.");
		return;
	}

	PlayerAnimInstance->PlayMontage(ActionType);
	Super::Start(InstigatorActor);
}

void UBAction_PrimaryAttack::Stop(AActor* InstigatorActor)
{
	Clear();
	Super::Stop(InstigatorActor);
}

void UBAction_PrimaryAttack::Clear()
{
	SectionIndex = 0;
}

void UBAction_PrimaryAttack::OnNextPrimaryAttackCheck()
{
	if (false == IsRunning())
	{
		return;
	}

	SectionIndex = (SectionIndex + 1) % MaxSectionIndex;

	ABPlayer* Player = Cast<ABPlayer>(GetOwningActionComponent()->GetOwner());
	if (nullptr == Player)
	{
		B_ASSERT_DEV(false, "�÷��̾ �����ϴ�.");
		return;
	}

	UBPlayerAnimInstance* PlayerAnimInstance = Player->GetPlayerAnimInstance();
	if (nullptr == PlayerAnimInstance)
	{
		B_ASSERT_DEV(false, "AnimInstance �� �����ϴ�.");
		return;
	}

	PlayerAnimInstance->MontageJumpToSection(ActionType, SectionIndex);
}

void UBAction_PrimaryAttack::OnPrimaryAttackHitCheck()
{
	if (false == IsRunning())
	{
		return;
	}

	ABPlayer* Player = Cast<ABPlayer>(GetOwningActionComponent()->GetOwner());
	if (nullptr == Player)
	{
		B_ASSERT_DEV(false, "�÷��̾ �����ϴ�.");
		return;
	}

	const FVector Direction = Player->GetActorForwardVector();
	const FVector ActionStartLocation = Player->GetActorLocation() + Direction * ActionOffset;
	const FVector ActionEndLocation = ActionStartLocation + Direction * ActionRange;

	FCollisionShape Shape;
	Shape.SetSphere(SweepRadius);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Player);

	FHitResult Hit;

	const bool Result = GetWorld()->SweepSingleByChannel(Hit, ActionStartLocation, ActionEndLocation, FQuat::Identity, ECC_GameTraceChannel1, Shape, Params);

	if (true == Result)
	{

		if (false == Hit.Actor.IsValid())
		{
			return;
		}

		UBStatusComponent* StatusComp = UBStatusComponent::GetStatus(Hit.Actor.Get());
		if (StatusComp)
		{
			StatusComp->ChangeHealth(Player, -Damage);
		}
	}

	if (PrintPrimaryAttack)
	{
		const FVector RangeCenter = (ActionStartLocation + ActionEndLocation) * 0.5f;
		const float HalfLength = ActionRange * 0.5f + SweepRadius;
		FColor DrawColor = true == Result ? FColor::Green : FColor::Red;

		DrawDebugCapsule(GetWorld()
			, RangeCenter
			, HalfLength
			, SweepRadius
			, FRotationMatrix::MakeFromZ(Direction).ToQuat()
			, DrawColor
			, false
			, 5.0f);
	}

}