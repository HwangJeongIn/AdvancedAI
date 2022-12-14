// Fill out your copyright notice in the Description page of Project Settings.


#include "BPlayer.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "BPlayerMovementComponent.h"
#include "BStatusComponent.h"
#include "BActionComponent.h"
#include "BPlayerAnimInstance.h"
#include "BPlayerController.h"
#include "BAction.h"


ABPlayer::ABPlayer()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	SetReplicateMovement(false);

	NetUpdateFrequency = 1.0f;
	MinNetUpdateFrequency = 1.0f;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;

	/** Scene */
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

		MeshComp->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
		MeshComp->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}

	/** Camera, SpringArm */
	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>("SpringArmComp");
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	SpringArmComp->TargetArmLength = 600.0f;
	SpringArmComp->SetupAttachment(RootComponent);

	// We control the rotation of spring arm with pawn control rotation already, this disables a subtle side effect
	// where rotating our CapsuleComp (eg. caused by bOrientRotationToMovement in Character Movement) will rotate our spring arm until it self corrects later in the update
	// This may cause unwanted effects when using CameraLocation during Tick as it may be slightly offset from our final camera position.
	SpringArmComp->SetUsingAbsoluteRotation(true);

	CameraComp = CreateDefaultSubobject<UCameraComponent>("CameraComp");
	CameraComp->SetupAttachment(SpringArmComp);

	/** Movement */
	PlayerMovementComp = CreateDefaultSubobject<UBPlayerMovementComponent>("PlayerMovementComp");
	ForwardMovementFactor = 0.0f;
	RightMovementFactor = 0.0f;

	/** Status */
	StatusComp = CreateDefaultSubobject<UBStatusComponent>("StatusComp");

	/** Action */
	ActionComp = CreateDefaultSubobject<UBActionComponent>("ActionComp");

}

void ABPlayer::BeginPlay()
{
	Super::BeginPlay();

	PlayerAnimInstance = Cast<UBPlayerAnimInstance>(MeshComp->GetAnimInstance());

	B_ASSERT_DEV(PlayerAnimInstance, " ????????????. ")
	if (PlayerAnimInstance)
	{
		PlayerAnimInstance->OnMontageEnded.AddDynamic(this, &ABPlayer::OnPrimaryAttackMontageEnded);
		InitializeActionComponent();
	}

	StatusComp->OnHealthChanged.AddDynamic(this, &ABPlayer::OnHealthChanged);
}

void ABPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABPlayer::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABPlayer::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);


	PlayerInputComponent->BindAction("PrimaryAttack", IE_Pressed, this, &ABPlayer::PrimaryAttackStart);
	PlayerInputComponent->BindAction("PrimaryAttack", IE_Released, this, &ABPlayer::PrimaryAttackStop);
	// Used generic name 'SecondaryAttack' for binding
	PlayerInputComponent->BindAction("SecondaryAttack", IE_Pressed, this, &ABPlayer::SecondaryAttack);
	PlayerInputComponent->BindAction("Dash", IE_Pressed, this, &ABPlayer::Dash);
	PlayerInputComponent->BindAction("Sprint", IE_Pressed, this, &ABPlayer::SprintStart);
	PlayerInputComponent->BindAction("Sprint", IE_Released, this, &ABPlayer::SprintEnd);


}

FVector ABPlayer::GetVelocity() const
{
	if (PlayerMovementComp)
	{
		return PlayerMovementComp->GetVelocity();
	}

	return FVector::ZeroVector;
}


float ABPlayer::GetCurrentYaw() const
{
	if (PlayerMovementComp)
	{
		return PlayerMovementComp->GetCurrentYaw();
	}

	return 0.0f;
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

void ABPlayer::OnHealthChanged(AActor* InstigatorActor, UBStatusComponent* OwningStatusComp, float NewHealth, float DeltaHealth)
{
	if (0.0f >= NewHealth)
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		DisableInput(PC);
		SetLifeSpan(5.0f);
	}
}

UBPlayerAnimInstance* ABPlayer::GetPlayerAnimInstance()
{
	return PlayerAnimInstance;
}

void ABPlayer::InitializeActionComponent()
{
	ActionComp->InitializeActions();
}

void ABPlayer::PrimaryAttackStart()
{
	ActionComp->StartAction(this, EActionType::PrimaryAttack);
}

void ABPlayer::PrimaryAttackStop()
{
	ActionComp->StopAction(this, EActionType::PrimaryAttack);
}

void ABPlayer::SecondaryAttack()
{

}

void ABPlayer::Dash()
{

}

void ABPlayer::SprintStart()
{
	ActionComp->StartAction(this, EActionType::Sprint);
}

void ABPlayer::SprintEnd()
{
	ActionComp->StopAction(this, EActionType::Sprint);
}

void ABPlayer::OnPrimaryAttackMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{

}