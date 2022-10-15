// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BSightComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"


static int32 PrintSight = 0;
FAutoConsoleVariableRef CVARDebugPrintSight(
	TEXT("B.PrintSight"),
	PrintSight,
	TEXT("Print Sight"),
	ECVF_Cheat);

UBSightComponent::UBSightComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	SetSightRadius(1000.0f);
	SetSightAngle(45.0f);
	IsEnabled = true;
}

void UBSightComponent::BeginPlay()
{
	Super::BeginPlay();

	// 테스트
	if (PrintSight)
	{
		OnSee.AddDynamic(this, &UBSightComponent::OnSeeTest);
	}

	// 에디터에서 변경했을 때 Cosine 갱신
	SetSightAngle(SightAngle);

	if (true == IsEnabled)
	{
		IsEnabled = false;
		SetIsEnabled(true);
	}
}

void UBSightComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	if (PrintSight)
	{
		AActor* OwnerActor = GetOwner();
		UWorld* OwnerActorWorld = OwnerActor->GetWorld();
		FVector CurrentActorLocation = OwnerActor->GetActorLocation();
		FRotator CurrentActorRotation = OwnerActor->GetActorRotation();
		FVector CurrentActorDirection = CurrentActorRotation.Vector();
		DrawDebugSphere(OwnerActorWorld, CurrentActorLocation, SightRadius, 12, FColor::Magenta, false, .5f);

		FRotator Rot1(0, SightAngle, 0);
		FRotator Rot2(0, -SightAngle, 0);

		FVector Pos1 = CurrentActorLocation + Rot1.RotateVector(CurrentActorDirection) * SightRadius;
		FVector Pos2 = CurrentActorLocation + Rot2.RotateVector(CurrentActorDirection) * SightRadius;
		
		DrawDebugLine(OwnerActorWorld, CurrentActorLocation, Pos1, FColor::Black, false, .5f);
		DrawDebugLine(OwnerActorWorld, CurrentActorLocation, Pos2, FColor::Black, false, .5f);
	}
}

void UBSightComponent::SetIsEnabled(bool NewIsEnabled)
{
	if (IsEnabled == NewIsEnabled)
	{
		return;
	}

	IsEnabled = NewIsEnabled;
	if (true == IsEnabled)
	{
		SetUpdateSeeingTimer(SeeingInterval);
	}
}

void UBSightComponent::SetSightRadius(float NewSightRadius)
{
	if (0 >= NewSightRadius)
	{
		B_ASSERT_DEV(false, "비정상적인 값입니다.");
		return;
	}

	SightRadius = NewSightRadius;
	SightRadiusSquared = FMath::Square(SightRadius);
}

void UBSightComponent::SetSightAngle(float NewSightAngle)
{
	SightAngle = NewSightAngle;
	SightAngleCosine = FMath::Cos(FMath::DegreesToRadians(SightAngle));
}

void UBSightComponent::SetUpdateSeeingTimer(float InSeeingInterval)
{
	const AActor* OwnerActor = GetOwner();
	if (false == IsValid(OwnerActor) 
		|| false == OwnerActor->HasAuthority())
	{
		return;
	}

	if (0 >= InSeeingInterval)
	{
		InSeeingInterval = KINDA_SMALL_NUMBER;
	}

	OwnerActor->GetWorldTimerManager().SetTimer(UpdateSeeingTimerHandle, this, 
		&UBSightComponent::OnUpdateSeeing, InSeeingInterval, false);
	
}

void UBSightComponent::SetSeeingInterval(float NewSeeingInterval)
{
	if (SeeingInterval == NewSeeingInterval)
	{
		return;
	}

	const float OldSeeingInterval = SeeingInterval;
	SeeingInterval = FMath::Max(0.0f, NewSeeingInterval);
	
	AActor* const OwnerActor = GetOwner();
	if (false == IsValid(OwnerActor))
	{
		return;
	}

	if (false == IsEnabled)
	{
		return;
	}

	if (SeeingInterval <= 0.0f)
	{
		SetUpdateSeeingTimer(0.0f);
	}
	else
	{
		float CurrentElapsed = OwnerActor->GetWorldTimerManager().GetTimerElapsed(UpdateSeeingTimerHandle);
		if (-1.0f == CurrentElapsed)
		{
			B_ASSERT_DEV(false, "비정상적인 상황입니다.");
			return;
		}

		float Remaining = OldSeeingInterval - CurrentElapsed;
		// - 값은 0.0f으로 변경된다.
		Remaining = FMath::Max(0.0f, Remaining);

		// 바뀐 간격보다 더 많이 남았을 때 타이머를 취소하고 새롭게 등록한다.
		// 더 적게 남은 경우에는 한번은 그냥 처리하고, 다음 타이머부터 간격을 적용한다.
		if (SeeingInterval < Remaining)
		{
			OwnerActor->GetWorldTimerManager().ClearTimer(UpdateSeeingTimerHandle);
			SetUpdateSeeingTimer(SeeingInterval);
		}
	}
}

void UBSightComponent::OnSeeTest(AActor* Target)
{
	B_ASSERT_DEV(false, "I'm seeing [%s]", *GetNameSafe(Target));
}

void UBSightComponent::TrySeeing()
{
	if (false == OnSee.IsBound())
	{
		return;
	}

	const AActor* OwnerActor = GetOwner();
	if (false == IsValid(OwnerActor))
	{
		return;
	}

	if (nullptr == TargetType)
	{
		for (FConstPlayerControllerIterator It = OwnerActor->GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			const APlayerController* PC = It->Get();
			if (false == IsValid(PC))
			{
				continue;
			}

			AActor* CurrentPlayerActor = PC->GetPawn();
			if (false == IsValid(CurrentPlayerActor)
				|| CurrentPlayerActor == OwnerActor)
			{
				continue;
			}

			ValidateTargetAndNotify(CurrentPlayerActor);
		}
	}
	else
	{
		for (TActorIterator<AActor> It(GetWorld(), TargetType); It; ++It)
		{
			AActor* CurrentActor = *It;
			if (false == IsValid(CurrentActor)
				|| CurrentActor == OwnerActor)
			{
				continue;
			}

			ValidateTargetAndNotify(CurrentActor);
		}

	}
}

void UBSightComponent::ValidateTargetAndNotify(AActor* Target)
{
	const AActor* OwnerActor = GetOwner();
	if (nullptr == OwnerActor)
	{
		return;
	}

	const FVector TargetLocation = Target->GetActorLocation();
	const FVector OwnerActorLocation = OwnerActor->GetActorLocation();
	const FVector OwnerActorToTarget = TargetLocation - OwnerActorLocation;

	// 거리 체크
	const float DistanceSq = OwnerActorToTarget.SizeSquared();
	if (DistanceSq > SightRadiusSquared)
	{
		return;
	}

	// 각도 체크
	const FVector OwnerActorToTargetDirection = OwnerActorToTarget.GetSafeNormal();
	const FVector OwnerActorDirection = OwnerActor->GetActorRotation().Vector();
	
	// 1 * 1 * cos(x)
	const float DotProductResult = FVector::DotProduct(OwnerActorToTargetDirection, OwnerActorDirection);
	if (SightAngleCosine > DotProductResult)
	{
		return;
	}

	OnSee.Broadcast(Target);
}

void UBSightComponent::OnUpdateSeeing()
{
	if (PrintSight)
	{
		B_LOG_DEV("OnUpdateSeeing");
	}

	const AActor* OwnerActor = GetOwner();
	if (false == IsValid(OwnerActor))
	{
		return;
	}

	TrySeeing();

	if (true == IsEnabled)
	{
		SetUpdateSeeingTimer(SeeingInterval);
	}
}