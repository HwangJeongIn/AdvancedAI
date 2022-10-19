// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BBTService_UpdateAIState.h"
#include "BStatusComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"



UBBTService_UpdateAIState::UBBTService_UpdateAIState()
{
	MaxDistanceToChase = 1000.0f;
	MaxDistanceToAttack = 150.0f;
}

/*
void UBBTService_UpdateAIState::SetMaxDistanceToChase(float InMaxDistanceToChase)
{
	MaxDistanceToChase = InMaxDistanceToChase;
	MaxDistanceSquaredToChase = MaxDistanceToChase * MaxDistanceToChase;
}
*/

void UBBTService_UpdateAIState::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* SelfController = OwnerComp.GetAIOwner();
	if (nullptr == SelfController)
	{
		B_ASSERT_DEV(false, " 비정상입니다 ");
		return;
	}

	APawn* SelfPawn = SelfController->GetPawn();
	if (nullptr == SelfPawn)
	{
		B_ASSERT_DEV(false, " 비정상입니다 ");
		return;
	}

	UBStatusComponent* StatusComp = UBStatusComponent::GetStatus(SelfPawn);
	if (nullptr == StatusComp)
	{
		B_ASSERT_DEV(false, " 비정상입니다 ");
		return;
	}

	UBlackboardComponent* BlackBoardComp = OwnerComp.GetBlackboardComponent();
	B_ASSERT_DEV(BlackBoardComp, " 비정상입니다.");

	/** HealthRate 갱신 */
	const float CurrentHealth = StatusComp->GetHealth();
	const float MaxHealth = StatusComp->GetMaxHealth();

	float HealthRate = (CurrentHealth / MaxHealth);
	HealthRate = FMath::Clamp(HealthRate, 0.0f, 1.0f);

	BlackBoardComp->SetValueAsFloat(HealthRateKey.SelectedKeyName, HealthRate);

	/** DistanceSquared 계산해서 CanAttack 등 갱신 */
	AActor* TargetActor = Cast<AActor>(BlackBoardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (TargetActor) // 없을 수 있다.
	{
		FVector ToTarget = TargetActor->GetActorLocation() - SelfPawn->GetActorLocation();
		ToTarget.Z = 0;
		const float DistanceSquared = ToTarget.SizeSquared();

		//B_LOG_DEV("%.1f, %.1f", DistanceSquared, MaxDistanceToAttack * MaxDistanceToAttack);
		if ((MaxDistanceToAttack * MaxDistanceToAttack) > DistanceSquared)
		{
			//B_LOG_DEV("1");
			BlackBoardComp->SetValueAsBool(CanAttackKey.SelectedKeyName, true);
		}
		else
		{
			//B_LOG_DEV("2");
			BlackBoardComp->SetValueAsBool(CanAttackKey.SelectedKeyName, false);

			if ((MaxDistanceToChase * MaxDistanceToChase) < DistanceSquared)
			{
				BlackBoardComp->SetValueAsObject(TargetActorKey.SelectedKeyName, nullptr);
			}
		}
	}
}