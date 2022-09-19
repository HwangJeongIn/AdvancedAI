
#include "BPlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
//#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"


// Sets default values
ABPlayerCharacter::ABPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArmComp = CreateDefaultSubobject<USpringArmComponent>("SpringArmComp");
	SpringArmComp->bUsePawnControlRotation = true;
	SpringArmComp->SetupAttachment(RootComponent);

	// We control the rotation of spring arm with pawn control rotation already, this disables a subtle side effect
	// where rotating our CapsuleComponent (eg. caused by bOrientRotationToMovement in Character Movement) will rotate our spring arm until it self corrects later in the update
	// This may cause unwanted effects when using CameraLocation during Tick as it may be slightly offset from our final camera position.
	SpringArmComp->SetUsingAbsoluteRotation(true);

	CameraComp = CreateDefaultSubobject<UCameraComponent>("CameraComp");
	CameraComp->SetupAttachment(SpringArmComp);

	GetCharacterMovement()->bOrientRotationToMovement = true;
	bUseControllerRotationYaw = false;

	// Enabled on mesh to react to incoming projectiles
	GetMesh()->SetGenerateOverlapEvents(true);
	// Disable on capsule collision to avoid double-dipping and receiving 2 overlaps when entering trigger zones etc.
	// Once from the mesh, and 2nd time from capsule
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);

	MovingFactor = 0.0f;
	RotationFactor = 0.0f;

	DefaultMovingForce = 10000;
	DefaultTurningRadius = 10;

	DragCoefficient = 16;
	FrictionCoefficient = 0.015;
}


void ABPlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ABPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	// 중력이 중간에 변한다면 실시간으로 계산해야한다.
	DefaultGravity = GetWorld()->GetGravityZ() / 100;
	DefaultMass = GetCharacterMovement()->Mass;
}

void ABPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UPrimitiveComponent* RootCompTemp = Cast<UPrimitiveComponent>(RootComponent);
	if (RootCompTemp)
	{

		FVector TestVel = GetVelocity();
		B_LOG_DEV("1 => %.1f, %.1f, %.1f", TestVel.X, TestVel.Y, TestVel.Z);
		GetCharacterMovement()->Velocity = (FVector(100,0,0));
		TestVel = GetVelocity();
		B_LOG_DEV("2 => %.1f, %.1f, %.1f", TestVel.X, TestVel.Y, TestVel.Z);
		
	}

	return;

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

}


FVector ABPlayerCharacter::GetPawnViewLocation() const
{
	return CameraComp->GetComponentLocation();
}


// Called to bind functionality to input
void ABPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABPlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABPlayerCharacter::MoveRight);

	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
}

float ABPlayerCharacter::GetAirResistance()
{
	// 마찰력 = (속도와 반대 방향) * 속도^2 * 마찰계수
	const FVector CurrentVelocity = GetVelocity();
	return CurrentVelocity.SizeSquared() * DragCoefficient;
}

float ABPlayerCharacter::GetFrictionResistance()
{
	// 공기저항 = (속도와 반대 방향) * 수직항력 * 항력계수 // 수직항력의 경우 수직으록 간주하고 계산한다. (M * G)
	const float NormalForce = DefaultMass * DefaultGravity;
	return FrictionCoefficient * NormalForce;
}


void ABPlayerCharacter::MoveForward(float Value)
{
	MovingFactor = Value;

	//GetCharacterMovement()->AddImpulse(finalMovingForce);
	//AddMovementInput(ControlRot.Vector(), Value);
}

void ABPlayerCharacter::MoveRight(float Value)
{
	FRotator ControlRot = GetControlRotation();
	ControlRot.Pitch = 0.0f;
	ControlRot.Roll = 0.0f;

	// X = Forward (Red)
	// Y = Right (Green)
	// Z = Up (Blue)

	FVector RightVector = FRotationMatrix(ControlRot).GetScaledAxis(EAxis::Y);

	AddMovementInput(RightVector, Value);
}