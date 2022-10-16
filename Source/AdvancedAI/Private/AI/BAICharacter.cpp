// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BAICharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "BStatusComponent.h"
#include "BActionComponent.h"
#include "AI/BSightComponent.h"



ABAICharacter::ABAICharacter()
{
	USkeletalMeshComponent* MeshComp = GetMesh();
	MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
	MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	
	/** Status */
	StatusComp = CreateDefaultSubobject<UBStatusComponent>("StatusComp");

	/** Action */
	ActionComp = CreateDefaultSubobject<UBActionComponent>("ActionComp");

	/** Sight */
	SightComp = CreateDefaultSubobject<UBSightComponent>("SightComp");
}

void ABAICharacter::BeginPlay()
{
	Super::BeginPlay();

	StatusComp->OnHealthChanged.AddDynamic(this, &ABAICharacter::OnHealthChanged);
	ActionComp->InitializeActions();
}

void ABAICharacter::OnHealthChanged(AActor* InstigatorActor, UBStatusComponent* OwningStatusComp, float NewHealth, float DeltaHealth)
{

}