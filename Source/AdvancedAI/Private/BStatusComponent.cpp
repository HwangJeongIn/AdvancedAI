// Fill out your copyright notice in the Description page of Project Settings.


#include "BStatusComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"


UBStatusComponent::UBStatusComponent()
{
	MaxHealth = 100;
	Health = MaxHealth;

	SetIsReplicatedByDefault(true);
}

UBStatusComponent* UBStatusComponent::GetStatus(AActor* Actor)
{
	if (nullptr == Actor)
	{
		return nullptr;
	}

	return Cast<UBStatusComponent>(Actor->GetComponentByClass(UBStatusComponent::StaticClass()));
}

bool UBStatusComponent::IsAliveActor(AActor* Actor)
{
	UBStatusComponent* StatusComp = UBStatusComponent::GetStatus(Actor);
	if (nullptr == StatusComp)
	{
		return false;
	}

	return StatusComp->IsAlive();
}

bool UBStatusComponent::IsAlive() const
{
	return (0.0f < Health);
}

bool UBStatusComponent::IsFullHealth() const
{
	return (MaxHealth == Health);
}

float UBStatusComponent::GetHealth() const
{
	return Health;
}

float UBStatusComponent::GetMaxHealth() const
{
	return MaxHealth;
}

bool UBStatusComponent::ChangeHealth(AActor * Instigator, float DeltaHealth)
{
	AActor* OwnerActor = GetOwner();

	if (false == OwnerActor->CanBeDamaged() && 0.0f >= DeltaHealth)
	{
		return false;
	}

	const float CurrentHealth = Health;
	const float NewHealth = FMath::Clamp(Health + DeltaHealth, 0.0f, MaxHealth);

	const float FinalDeltaHealth = NewHealth - CurrentHealth;
	if (0.0f == FinalDeltaHealth)
	{
		return false;
	}

	// 서버에서 한번 수행 > 클라이언트로 통보
	if (OwnerActor->HasAuthority())
	{
		Health = NewHealth;
		MulticastHealthChanged(Instigator, Health, FinalDeltaHealth);
		B_LOG_DEV("Current Health : %.1f", Health);
	}

	return true;
}

bool UBStatusComponent::ForceKill(AActor* Instigator)
{
	return ChangeHealth(Instigator, -GetMaxHealth());
}

void UBStatusComponent::MulticastHealthChanged_Implementation(AActor * Instigator, float NewHealth, float DeltaHealth)
{
	OnHealthChanged.Broadcast(Instigator, this, NewHealth, DeltaHealth);
}

void UBStatusComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>&OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBStatusComponent, Health);
	DOREPLIFETIME(UBStatusComponent, MaxHealth);
}