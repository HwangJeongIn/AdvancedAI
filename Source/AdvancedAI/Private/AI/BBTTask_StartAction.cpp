// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BBTTask_StartAction.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BStatusComponent.h"
#include "BActionComponent.h"
#include "BAction.h"



UBBTTask_StartAction::UBBTTask_StartAction()
{

}

EBTNodeResult::Type UBBTTask_StartAction::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* SelfController = OwnerComp.GetAIOwner();
	if (nullptr == SelfController)
	{
		B_ASSERT_DEV(false, " 비정상입니다 ");
		return EBTNodeResult::Failed;
	}

	APawn* SelfPawn = SelfController->GetPawn();
	if (nullptr == SelfPawn)
	{
		B_ASSERT_DEV(false, " 비정상입니다 ");
		return EBTNodeResult::Failed;
	}

	UBActionComponent* ActionComp = Cast<UBActionComponent>(SelfPawn->GetComponentByClass(UBActionComponent::StaticClass()));
	if (nullptr == ActionComp)
	{
		B_ASSERT_DEV(false, " Action이 존재하지 않습니다. ");
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BlackBoardComp = OwnerComp.GetBlackboardComponent();
	if (nullptr == BlackBoardComp)
	{
		B_ASSERT_DEV(BlackBoardComp, " 비정상입니다.");
		return EBTNodeResult::Failed;
	}

	AActor* TargetActor = Cast<AActor>(BlackBoardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (nullptr == TargetActor)
	{
		B_LOG_DEV(" 대상이 없습니다. ");
		return EBTNodeResult::Failed;
	}

	if (false == UBStatusComponent::IsAliveActor(TargetActor))
	{
		B_LOG_DEV(" 이미 사망한 대상입니다. ");
		return EBTNodeResult::Failed;
	}

	if (false == ActionComp->StartActionIfCan(SelfPawn, ActionType))
	{
		B_ASSERT_DEV(false, " 비정상입니다. ");
		return EBTNodeResult::Failed;
	}

	BlackBoardComp->SetValueAsEnum(CurrentActionTypeKey.SelectedKeyName, static_cast<uint8>(ActionType));

	return EBTNodeResult::Succeeded;
}





/*
FVector MuzzleLocation = MyPawn->GetMesh()->GetSocketLocation("Muzzle_01");
FVector Direction = TargetActor->GetActorLocation() - MuzzleLocation;
FRotator MuzzleRotation = Direction.Rotation();

// Ignore negative pitch to not hit the floor in front itself
MuzzleRotation.Pitch += FMath::RandRange(0.0f, MaxBulletSpread);
MuzzleRotation.Yaw += FMath::RandRange(-MaxBulletSpread, MaxBulletSpread);

FActorSpawnParameters Params;
Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
Params.Instigator = MyPawn;

AActor* NewProj = GetWorld()->SpawnActor<AActor>(ProjectileClass, MuzzleLocation, MuzzleRotation, Params);

return NewProj ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
*/

