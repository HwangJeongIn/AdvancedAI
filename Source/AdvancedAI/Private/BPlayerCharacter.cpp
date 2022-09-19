
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

	MinTurningRadius = 50.0f; // 50 cm
}


void ABPlayerCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
}

void ABPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	TargetYaw = GetActorRotation().Yaw;
}

void ABPlayerCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!MovingFactor && !RotationFactor)
	{
		return;
	}
	
	//B_LOG_DEV("%.1f, %.1f, %.1f", CurrentVelocity.X, CurrentVelocity.Y, CurrentVelocity.Z);
	//B_LOG_DEV("Test : %.1f", RotationAngle);
	
	const FRotator ControlRot = GetControlRotation();
	FRotator ActorRot = GetActorRotation();
	float ActorYaw = ActorRot.Yaw;
	if (0 > ActorYaw)
	{
		ActorYaw = 360 + ActorYaw;
	}

	// �� ȸ������ Yaw�� ���̰� ������ �������ش�. ���� ��, �ӵ��� ȸ���ݰ����� ����Ѵ�.
	const float RemainingYaw = ControlRot.Yaw - ActorYaw;
	const float RemainingYawPositive = FMath::Abs<float>(RemainingYaw);
	if (SMALL_NUMBER > RemainingYawPositive)
	{
		AddMovementInput(ActorRot.Vector(), MovingFactor);
	}
	else
	{
		B_LOG_DEV("=============================================================");
		B_LOG_DEV("ControlRot.Yaw : %.1f", ControlRot.Yaw);
		B_LOG_DEV("ActorRot.Yaw : % .1f", ActorRot.Yaw);
		B_LOG_DEV("ActorYaw : % .1f", ActorYaw);
		B_LOG_DEV("RemainingAngle : %.1f", RemainingYaw);

		const FVector CurrentVelocity = GetVelocity();
		float DeltaLocation = FVector::DotProduct(GetOwner()->GetActorForwardVector(), CurrentVelocity) * DeltaSeconds;
		float DeltaYaw = DeltaLocation / MinTurningRadius;// *RotationFactor;
		DeltaYaw = FMath::RadiansToDegrees<float>(DeltaYaw);
		DeltaYaw = FMath::Abs<float>(DeltaYaw);


		B_LOG_DEV("DeltaYaw : %.1f", DeltaYaw);

		float Ratio = 0.0f;

		if (DeltaYaw > RemainingYawPositive)
		{
			Ratio = 1.0f;
		}
		else
		{
			Ratio = DeltaYaw / RemainingYawPositive;
		}

		// �ִ� �Ÿ��� �����ϱ� ���� FQuat ������ ����Ѵ�.
		FQuat NewQuat = FQuat::Slerp(ActorRot.Quaternion(), ControlRot.Quaternion(), Ratio);

		FRotator NewRotator = NewQuat.Rotator();
		NewRotator.Pitch = 0.0f;
		NewRotator.Roll = 0.0f;
		SetActorRotation(NewRotator);
		AddMovementInput(NewRotator.Vector(), MovingFactor);
	}
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
	//PlayerInputComponent->BindAxis("Turn", this, &ABPlayerCharacter::AddControllerRotationInput);
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


void ABPlayerCharacter::AddControllerRotationInput(float Value)
{
	B_LOG_DEV("Test : %.1f", Value);
}