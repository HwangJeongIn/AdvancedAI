// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BAIController.h"
#include "BehaviorTree/BehaviorTree.h"


void ABAIController::BeginPlay()
{
	if (nullptr == BehaviorTree)
	{
		B_ASSERT_DEV(false, "�������Դϴ�.");
		return;
	}

	RunBehaviorTree(BehaviorTree);
}
