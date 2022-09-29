// Fill out your copyright notice in the Description page of Project Settings.


#include "BPlayer.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "BPlayerMovementComponent.h"


ABPlayer::ABPlayer()
{
	PrimaryActorTick.bCanEverTick = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	PlayerMovementComp = CreateDefaultSubobject<UBPlayerMovementComponent>("PlayerMovementComp");

	CapsuleComp = CreateDefaultSubobject<UCapsuleComponent>("CapsuleComp");
	CapsuleComp->InitCapsuleSize(34.0f, 88.0f);
	CapsuleComp->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);

	CapsuleComp->CanCharacterStepUpOn = ECB_No;
	CapsuleComp->SetShouldUpdatePhysicsVolume(true);
	CapsuleComp->SetCanEverAffectNavigation(false);
	CapsuleComp->bDynamicObstacle = true;
	RootComponent = CapsuleComp;

	MeshComp = CreateOptionalDefaultSubobject<USkeletalMeshComponent>("MeshComp");
	if (MeshComp)
	{
		MeshComp->AlwaysLoadOnClient = true;
		MeshComp->AlwaysLoadOnServer = true;
		MeshComp->bOwnerNoSee = false;
		MeshComp->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
		MeshComp->bCastDynamicShadow = true;
		MeshComp->bAffectDynamicIndirectLighting = true;
		MeshComp->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		MeshComp->SetupAttachment(CapsuleComp);
		static FName MeshCollisionProfileName(TEXT("CharacterMesh"));
		MeshComp->SetCollisionProfileName(MeshCollisionProfileName);
		MeshComp->SetGenerateOverlapEvents(false);
		MeshComp->SetCanEverAffectNavigation(false);
	}

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>("SpringArmComp");
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);

	// We control the rotation of spring arm with pawn control rotation already, this disables a subtle side effect
	// where rotating our CapsuleComp (eg. caused by bOrientRotationToMovement in Character Movement) will rotate our spring arm until it self corrects later in the update
	// This may cause unwanted effects when using CameraLocation during Tick as it may be slightly offset from our final camera position.
	SpringArmComp->SetUsingAbsoluteRotation(true);

	CameraComp = CreateDefaultSubobject<UCameraComponent>("CameraComp");
	CameraComp->SetupAttachment(SpringArmComp);


	// 시뮬레이션 테스트
	ForwardMovementFactor = 0.0f;
	RightMovementFactor = 0.0f;
}

// Called when the game starts or when spawned
void ABPlayer::BeginPlay()
{
	Super::BeginPlay();

}
/*
void ABPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
*/

void ABPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABPlayer::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABPlayer::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
}

FVector ABPlayer::GetVelocity() const
{
	if (PlayerMovementComp)
	{
		return PlayerMovementComp->GetPlayerVelocity();
	}

	return FVector::ZeroVector;
}

float ABPlayer::GetForwardMovementFactor() const
{
	return ForwardMovementFactor;
}

float ABPlayer::GetRightMovementFactor() const
{
	return RightMovementFactor;
}

void ABPlayer::MoveForward(float Value)
{
	ForwardMovementFactor = Value;
	/*
	FRotator ControlRot = GetControlRotation();
	ControlRot.Pitch = 0.0f;
	ControlRot.Roll = 0.0f;

	AddMovementInput(ControlRot.Vector(), Value);
	*/
}

void ABPlayer::MoveRight(float Value)
{
	RightMovementFactor = Value;

	/*
	FRotator ControlRot = GetControlRotation();
	ControlRot.Pitch = 0.0f;
	ControlRot.Roll = 0.0f;

	FVector RightVector = FRotationMatrix(ControlRot).GetScaledAxis(EAxis::Y);
	*/
}

