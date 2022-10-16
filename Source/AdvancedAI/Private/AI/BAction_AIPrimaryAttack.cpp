// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BAction_AIPrimaryAttack.h"
#include "GameFramework/Character.h"
#include "AI/BAIAnimInstance.h"
#include "BActionComponent.h"
#include "BStatusComponent.h"
#include "DrawDebugHelpers.h"


static int32 PrintAIPrimaryAttack = 1;
FAutoConsoleVariableRef CVARDebugPrintAIPrimaryAttack(
	TEXT("B.PrintAIPrimaryAttack"),
	PrintAIPrimaryAttack,
	TEXT("Print AI Primary Attack"),
	ECVF_Cheat);


void UBAction_AIPrimaryAttack::Initialize(UBActionComponent* NewActionComp)
{
	Super::Initialize(NewActionComp);

	ACharacter* AICharacter = Cast<ACharacter>(NewActionComp->GetOwner());
	if (nullptr == AICharacter)
	{
		B_ASSERT_DEV(false, " AI Character 가 없습니다.");
		return;
	}

	UBAIAnimInstance* AIAnimInstance = Cast<UBAIAnimInstance>(AICharacter->GetMesh()->GetAnimInstance());
	AIAnimInstance->OnAIPrimaryAttackHit.AddDynamic(this, &UBAction_AIPrimaryAttack::OnAIPrimaryAttackHit);
}

void UBAction_AIPrimaryAttack::Start(AActor* InstigatorActor)
{
	ACharacter* AICharacter = Cast<ACharacter>(InstigatorActor);
	if (AICharacter)
	{
		UBAIAnimInstance* AIAnimInstance = Cast<UBAIAnimInstance>(AICharacter->GetMesh()->GetAnimInstance());
		if (AIAnimInstance)
		{
			AIAnimInstance->PlayMontage(ActionType);
		}
	}

	Super::Start(InstigatorActor);
}

void UBAction_AIPrimaryAttack::Stop(AActor* InstigatorActor)
{
	Super::Stop(InstigatorActor);
}

void UBAction_AIPrimaryAttack::OnAIPrimaryAttackHit()
{
	if (false == IsRunning())
	{
		return;
	}

	ACharacter* AICharacter = Cast<ACharacter>(GetOwningActionComponent()->GetOwner());
	if (nullptr == AICharacter)
	{
		B_ASSERT_DEV(false, " AI Character 가 없습니다.");
		return;
	}

	const FVector Direction = AICharacter->GetActorForwardVector();
	const FVector ActionStartLocation = AICharacter->GetActorLocation() + Direction * ActionOffset;
	const FVector ActionEndLocation = ActionStartLocation + Direction * ActionRange;

	FCollisionShape Shape;
	Shape.SetSphere(SweepRadius);

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(AICharacter);

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
			StatusComp->ChangeHealth(AICharacter, -Damage);
		}
	}

	if (PrintAIPrimaryAttack)
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