// Fill out your copyright notice in the Description page of Project Settings.


#include "BAction.h"

#include "BActionComponent.h"
//#include "../ActionRoguelike.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"


bool UBAction::IsSupportedForNetworking() const
{
	return true;
}

UWorld* UBAction::GetWorld() const
{
	AActor* Actor = Cast<AActor>(GetOuter());
	if (Actor)
	{
		return Actor->GetWorld();
	}

	return nullptr;
}

void UBAction::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBAction, BasicRunningData);
	DOREPLIFETIME(UBAction, ActionComp);
}

void UBAction::Initialize(UBActionComponent* NewActionComp)
{
	ActionComp = NewActionComp;
}

UBActionComponent* UBAction::GetOwningActionComponent() const
{
	return ActionComp;
}

bool UBAction::IsRunning() const
{
	return (true == BasicRunningData.IsRunning);
}

bool UBAction::CanStart(AActor* InstigatorActor) const
{
	return (false == IsRunning());
}

bool UBAction::CanStop(AActor* InstigatorActor) const
{
	return (true == IsRunning());
}

void UBAction::Start(AActor* InstigatorActor)
{
	B_LOG_DEV("Started: %s", *GetNameSafe(this));

	UBActionComponent* Comp = GetOwningActionComponent();

	BasicRunningData.IsRunning = true;
	BasicRunningData.InstigatorActor = InstigatorActor;

	Comp->OnActionStarted.Broadcast(Comp, this);
}

void UBAction::Stop(AActor* InstigatorActor)
{
	B_LOG_DEV("Stopped: %s", *GetNameSafe(this));

	UBActionComponent* Comp = GetOwningActionComponent();

	BasicRunningData.IsRunning = false;
	BasicRunningData.InstigatorActor = InstigatorActor;

	Comp->OnActionStopped.Broadcast(Comp, this);
}

bool UBAction::Compare(const FName& InputName) const
{
	return (InputName == ActionName);
}

void UBAction::OnRep_BasicRunningData()
{
	if (BasicRunningData.IsRunning)
	{
		Start(BasicRunningData.InstigatorActor);
	}
	else
	{
		Stop(BasicRunningData.InstigatorActor);
	}
}