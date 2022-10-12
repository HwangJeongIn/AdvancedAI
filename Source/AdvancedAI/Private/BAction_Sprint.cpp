// Fill out your copyright notice in the Description page of Project Settings.


#include "BAction_Sprint.h"
#include "BPlayer.h"
#include "BPlayerMovementComponent.h"



void UBAction_Sprint::Start(AActor* InstigatorActor)
{
	ABPlayer* Target = Cast<ABPlayer>(InstigatorActor);
	if (Target)
	{
		UBPlayerMovementComponent* PlayerMovement = Cast<UBPlayerMovementComponent>(Target->GetComponentByClass(UBPlayerMovementComponent::StaticClass()));
		if (PlayerMovement)
		{
			PlayerMovement->SetMaxVelocityFactor(2.0f);
		}
	}

	Super::Start(InstigatorActor);
}

void UBAction_Sprint::Stop(AActor* InstigatorActor)
{
	ABPlayer* Target = Cast<ABPlayer>(InstigatorActor);
	if (Target)
	{
		UBPlayerMovementComponent* PlayerMovement = Cast<UBPlayerMovementComponent>(Target->GetComponentByClass(UBPlayerMovementComponent::StaticClass()));
		if (PlayerMovement)
		{
			PlayerMovement->SetMaxVelocityFactor(1.0f);
		}
	}

	Super::Stop(InstigatorActor);
}