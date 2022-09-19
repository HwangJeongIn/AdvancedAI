
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

	MinTurningRadius = 10.0f;
}


void ABPlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ABPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

}

void ABPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	const FVector CurrentVelocity = GetVelocity();
	float DeltaLocation = FVector::DotProduct(GetOwner()->GetActorForwardVector(), CurrentVelocity) * DeltaSeconds;
	float RotationAngle = DeltaLocation / MinTurningRadius * RotationFactor;
	FQuat RotationDelta(GetOwner()->GetActorUpVector(), RotationAngle);

	//Velocity = RotationDelta.RotateVector(Velocity);

	GetOwner()->AddActorWorldRotation(RotationDelta);

	FRotator ControlRot = GetControlRotation();
	ControlRot.Pitch = 0.0f;
	ControlRot.Roll = 0.0f;

	AddMovementInput(ControlRot.Vector(), MovingFactor);
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

void ABPlayerCharacter::MoveForward(float Value)
{
	MovingFactor = Value;

	//B_LOG_DEV("MoveForward");
}

void ABPlayerCharacter::MoveRight(float Value)
{
	RotationFactor = Value;

	//B_LOG_DEV("MoveRight");
}