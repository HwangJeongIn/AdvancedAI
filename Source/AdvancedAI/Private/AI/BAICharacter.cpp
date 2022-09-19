// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BAICharacter.h"

// Sets default values
ABAICharacter::ABAICharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABAICharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABAICharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void ABAICharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

