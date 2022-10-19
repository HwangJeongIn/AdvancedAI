// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BAIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AI/BSightComponent.h"



ABAIController::ABAIController()
{
	TargetActorKeyName = "TargetActor";
	OriginKeyName = "Origin";
}

void ABAIController::BeginPlay()
{
	if (nullptr == BehaviorTree)
	{
		B_ASSERT_DEV(false, "�������Դϴ�.");
		return;
	}

	RunBehaviorTree(BehaviorTree);

	UBlackboardComponent* BlackBoardComp = GetBlackboardComponent();
	if (nullptr == BlackBoardComp)
	{
		B_ASSERT_DEV(false, " �������Դϴ�.");
	}
	else
	{
		APawn* SelfPawn = GetPawn();
		if (nullptr == SelfPawn)
		{
			B_ASSERT_DEV(false, " �������Դϴ�.");
		}
		else
		{
			BlackBoardComp->SetValueAsVector(OriginKeyName, GetPawn()->GetActorLocation());
		}
	}
}

void ABAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	
	/** Ÿ��, ��������, �̺�Ʈ ���ε� */
	SetTargetActor(nullptr);
	CachedSelfPawn = InPawn;

	UBSightComponent* SightComp = Cast<UBSightComponent>(InPawn->GetComponentByClass(UBSightComponent::StaticClass()));
	if (nullptr != SightComp)
	{
		B_LOG_DEV("SightComp->OnSee.AddDynamic");
		SightComp->OnSee.AddDynamic(this, &ABAIController::OnSenseTarget);
	}
	else
	{
		B_LOG_DEV("SightComp�� ���� AI �Դϴ�. %s", *GetNameSafe(InPawn));
	}
}

void ABAIController::OnUnPossess()
{
	Super::OnUnPossess();

	AActor* PrevSelfPawn = CachedSelfPawn.Get();
	if (nullptr != PrevSelfPawn)
	{
		UBSightComponent* SightComp = Cast<UBSightComponent>(PrevSelfPawn->GetComponentByClass(UBSightComponent::StaticClass()));
		if (nullptr != SightComp)
		{
			B_LOG_DEV("SightComp->OnSee.RemoveDynamic");
			SightComp->OnSee.RemoveDynamic(this, &ABAIController::OnSenseTarget);
		}
	}
}

AActor* ABAIController::GetTargetActor()
{
	return TargetActor.Get();
}

void ABAIController::SetTargetActor(AActor* InTargetActor)
{
	TargetActor = InTargetActor;
}

void ABAIController::OnSenseTarget(AActor* InTargetActor)
{
	AActor* CurrentTargetActor = GetTargetActor();
	if (nullptr != CurrentTargetActor)
	{
		return;
	}

	if (false == IsValid(InTargetActor))
	{
		return;
	}

	UBlackboardComponent* BlackBoardComp = GetBlackboardComponent();
	if (nullptr != BlackBoardComp)
	{
		BlackBoardComp->SetValueAsObject(TargetActorKeyName, InTargetActor);
	}
}

