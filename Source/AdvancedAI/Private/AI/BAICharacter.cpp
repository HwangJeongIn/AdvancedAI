// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BAICharacter.h"
#include "BStatusComponent.h"
#include "BActionComponent.h"


ABAICharacter::ABAICharacter()
{
	/** Status */
	StatusComp = CreateDefaultSubobject<UBStatusComponent>("StatusComp");

	/** Action */
	ActionComp = CreateDefaultSubobject<UBActionComponent>("ActionComp");
}

void ABAICharacter::BeginPlay()
{
	Super::BeginPlay();

	StatusComp->OnHealthChanged.AddDynamic(this, &ABAICharacter::OnHealthChanged);
}

void ABAICharacter::OnHealthChanged(AActor* InstigatorActor, UBStatusComponent* OwningStatusComp, float NewHealth, float DeltaHealth)
{

}