// Fill out your copyright notice in the Description page of Project Settings.


#include "BActionComponent.h"
#include "BAction.h"
//#include "../ActionRoguelike.h"
#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "GameFramework/Actor.h"



UBActionComponent::UBActionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	IsInitialized = false;
	SetIsReplicatedByDefault(true);
}

bool UBActionComponent::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	for (UBAction* Action : ActivatedActions)
	{
		if (Action)
		{
			WroteSomething |= Channel->ReplicateSubobject(Action, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

void UBActionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBActionComponent, ActivatedActions);
}

void UBActionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	return;
	//FString DebugMsg = GetNameSafe(GetOwner()) + " : " + ActiveGameplayTags.ToStringSimple();
	//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, DebugMsg);

	// Draw All ActivatedActions
 	for (UBAction* Action : ActivatedActions)
 	{
 		FString ActionMsg = FString::Printf(TEXT("[%s]() Action: %s"), *GetNameSafe(GetOwner()), *GetNameSafe(Action));
		B_LOG_DEV("%d => %s", Action->IsRunning(), *ActionMsg);
 	}
}

void UBActionComponent::BeginPlay()
{
	Super::BeginPlay();

	AActor* OwnerActor = GetOwner();
	if (true == OwnerActor->HasAuthority())
	{
		for (const TSubclassOf<UBAction>& ActionClass : DefaultActionClasses)
		{
			AddAction(OwnerActor, ActionClass);
		}
	}
}

void UBActionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (UBAction* Action : ActivatedActions)
	{
		B_ASSERT_DEV(Action, "왜 발생하는 지 확인해야합니다.");
		if (nullptr == Action || false == Action->IsRunning())
		{
			continue;
		}

		Action->Stop(GetOwner());
	}

	Super::EndPlay(EndPlayReason);
}

void UBActionComponent::InitializeActions()
{
	for (UBAction* Action : ActivatedActions)
	{
		B_ASSERT_DEV(Action, "왜 발생하는 지 확인해야합니다.");
		if (nullptr == Action)
		{
			continue;
		}

		Action->Initialize(this);
	}

	IsInitialized = true;
}

void UBActionComponent::AddAction(AActor* Instigator, TSubclassOf<UBAction> ActionClass)
{
	if (nullptr == ActionClass)
	{
		B_ASSERT_DEV(false, "비정상입니다.");
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (false == OwnerActor->HasAuthority())
	{
		B_ASSERT_DEV(false, "클라이언트 입니다.");
		return;
	}

	UBAction* NewAction = NewObject<UBAction>(OwnerActor, ActionClass);
	if (nullptr == NewAction)
	{
		B_ASSERT_DEV(false, "비정상입니다.");
		return;
	}

	if (true == IsInitialized)
	{
		NewAction->Initialize(this);
	}
	ActivatedActions.Add(NewAction);
}

void UBActionComponent::RemoveAction(UBAction* ActionToRemove)
{
	if (nullptr == ActionToRemove || true == ActionToRemove->IsRunning())
	{
		return;
	}

	ActivatedActions.Remove(ActionToRemove);
}

UBAction* UBActionComponent::GetAction(TSubclassOf<UBAction> ActionClass) const
{
	for (UBAction* Action : ActivatedActions)
	{
		B_ASSERT_DEV(Action, "왜 발생하는 지 확인해야합니다.");
		if (nullptr == Action || false == Action->IsA(ActionClass))
		{
			continue;
		}

		return Action;
	}

	return nullptr;
}

UBAction* UBActionComponent::GetActionByName(const FName& ActionName) const
{
	for (UBAction* Action : ActivatedActions)
	{
		B_ASSERT_DEV(Action, "왜 발생하는 지 확인해야합니다.");
		if (nullptr == Action || false == Action->Compare(ActionName))
		{
			continue;
		}

		return Action;
	}

	return nullptr;
}

bool UBActionComponent::StartActionByName(AActor* Instigator, const FName& ActionName, bool WithoutActionStateValidation /*= false*/)
{
	if (false == IsInitialized)
	{
		return false;
	}

	UBAction* TargetAction = GetActionByName(ActionName);
	if (nullptr == TargetAction)
	{
		B_ASSERT_DEV(false, "[%s]에 [%s] 액션이 존재하지 않습니다.", *GetNameSafe(Instigator), *ActionName.ToString());
		return false;
	}

	if (false == TargetAction->CanStart(Instigator))
	{
		if (false == WithoutActionStateValidation)
		{
			B_ASSERT_DEV(false, "[%s]가 [%s] 액션을 이미 진행하고 있습니다.", *GetNameSafe(Instigator), *ActionName.ToString());
			return false;
		}
		return true;
	}

	if (false == GetOwner()->HasAuthority())
	{
		ServerStartAction(Instigator, ActionName);
	}

	TargetAction->Start(Instigator);
	return true;
}

bool UBActionComponent::StartActionByNameIfCan(AActor* Instigator, const FName& ActionName)
{
	return StartActionByName(Instigator, ActionName, true);
}

bool UBActionComponent::StopActionByName(AActor* Instigator, const FName& ActionName, bool WithoutActionStateValidation /*= false*/)
{
	if (false == IsInitialized)
	{
		return false;
	}

	UBAction* TargetAction = GetActionByName(ActionName);
	if (nullptr == TargetAction)
	{
		B_ASSERT_DEV(false, "[%s]에 [%s] 액션이 존재하지 않습니다.", *GetNameSafe(Instigator), *ActionName.ToString());
		return false;
	}

	if (false == TargetAction->CanStop(Instigator))
	{
		if (false == WithoutActionStateValidation)
		{
			B_ASSERT_DEV(false, "[%s]가 [%s] 액션을 진행하고 있지 않습니다.", *GetNameSafe(Instigator), *ActionName.ToString());
			return true;
		}
		return false;
	}

	if (false == GetOwner()->HasAuthority())
	{
		ServerStopAction(Instigator, ActionName);
	}

	TargetAction->Stop(Instigator);
	return true;
}

bool UBActionComponent::StopActionByNameIfCan(AActor* Instigator, const FName& ActionName)
{
	return StopActionByName(Instigator, ActionName, true);
}


void UBActionComponent::ServerStartAction_Implementation(AActor* Instigator, FName ActionName)
{
	StartActionByName(Instigator, ActionName);
}


void UBActionComponent::ServerStopAction_Implementation(AActor* Instigator, FName ActionName)
{
	StopActionByName(Instigator, ActionName);
}

