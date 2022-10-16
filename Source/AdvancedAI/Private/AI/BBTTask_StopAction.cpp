// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BBTTask_StopAction.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BStatusComponent.h"
#include "BActionComponent.h"



UBBTTask_StopAction::UBBTTask_StopAction()
{

}

EBTNodeResult::Type UBBTTask_StopAction::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* SelfController = OwnerComp.GetAIOwner();
	if (nullptr == SelfController)
	{
		B_ASSERT_DEV(false, " �������Դϴ� ");
		return EBTNodeResult::Failed;
	}

	APawn* SelfPawn = SelfController->GetPawn();
	if (nullptr == SelfPawn)
	{
		B_ASSERT_DEV(false, " �������Դϴ� ");
		return EBTNodeResult::Failed;
	}

	UBActionComponent* ActionComp = Cast<UBActionComponent>(SelfPawn->GetComponentByClass(UBActionComponent::StaticClass()));
	if (nullptr == ActionComp)
	{
		B_ASSERT_DEV(false, " Action�� �������� �ʽ��ϴ�. ");
		return EBTNodeResult::Failed;
	}

	if (false == ActionComp->StopActionByNameIfCan(SelfPawn, ActionName))
	{
		B_ASSERT_DEV(false, " �������Դϴ�. ");
		return EBTNodeResult::Failed;
	}

	return EBTNodeResult::Succeeded;
}
