// Fill out your copyright notice in the Description page of Project Settings.


#include "BPlayer.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"


ABPlayer::ABPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = true;


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
	/*
	MovingFactor = 0.0f;
	RotationFactor = 0.0f;

	DefaultMovingForce = 10000;
	DefaultTurningRadius = 10;

	DragCoefficient = 16;
	FrictionCoefficient = 0.015;
	*/
}

// Called when the game starts or when spawned
void ABPlayer::BeginPlay()
{
	Super::BeginPlay();

	// 시뮬레이션 테스트
	/*
	// 중력이 중간에 변한다면 실시간으로 계산해야한다.
	DefaultGravity = GetWorld()->GetGravityZ() / 100;
	DefaultMass = GetCharacterMovement()->Mass;
	*/
}

// Called every frame
void ABPlayer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 시뮬레이션 테스트
	/*
	FRotator ControlRot = GetControlRotation();
	ControlRot.Pitch = 0.0f;
	ControlRot.Roll = 0.0f;

	FVector CharacterForward = ControlRot.Vector();

	float CurrentForceValue = DefaultMovingForce * MovingFactor;
	const float CurrentResistanceValue = GetAirResistance() + GetFrictionResistance();

	CurrentForceValue -= CurrentResistanceValue;

	FVector Acceleration = CharacterForward * CurrentForceValue / DefaultMass;
	FVector FinalVelocity = GetVelocity() + Acceleration * DeltaSeconds;
	UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(RootComponent);
	if (RootComp)
	{
		B_LOG_DEV("%.1f, %.1f, %.1f", FinalVelocity.X, FinalVelocity.Y, FinalVelocity.Z);
		RootComp->SetPhysicsLinearVelocity(FinalVelocity);
	}

	//B_LOG_DEV("%.1f, %.1f, %.1f", finalMovingForce.X, finalMovingForce.Y, finalMovingForce.Z);
	//GetCharacterMovement()->AddForce(finalMovingForce);
	*/
}

// 시뮬레이션 테스트
/*
float ABPlayerCharacter::GetAirResistance()
{
	// 마찰력 = (속도와 반대 방향) * 속도^2 * 마찰계수
	const FVector CurrentVelocity = GetVelocity();
	return CurrentVelocity.SizeSquared() * DragCoefficient;
}

float ABPlayerCharacter::GetFrictionResistance()
{
	// 공기저항 = (속도와 반대 방향) * 수직항력 * 항력계수 // 수직항력의 경우 지면을 수평으로 간주하고 계산한다. (M * G)
	const float NormalForce = DefaultMass * DefaultGravity;
	return FrictionCoefficient * NormalForce;
}
*/

// Called to bind functionality to input
void ABPlayer::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABPlayer::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABPlayer::MoveRight);
}

void ABPlayer::MoveForward(float Value)
{
	FRotator ControlRot = GetControlRotation();
	ControlRot.Pitch = 0.0f;
	ControlRot.Roll = 0.0f;

	AddMovementInput(ControlRot.Vector(), Value);
}

void ABPlayer::MoveRight(float Value)
{
	FRotator ControlRot = GetControlRotation();
	ControlRot.Pitch = 0.0f;
	ControlRot.Roll = 0.0f;

	FVector RightVector = FRotationMatrix(ControlRot).GetScaledAxis(EAxis::Y);
	AddMovementInput(RightVector, Value);
}

