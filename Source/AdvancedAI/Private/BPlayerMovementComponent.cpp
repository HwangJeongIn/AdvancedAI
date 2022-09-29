// Fill out your copyright notice in the Description page of Project Settings.


#include "BPlayerMovementComponent.h"
#include "Engine/World.h"
#include "BPlayer.h"	

static int32 PrintPlayerMovementComponentVelocity = 1;
FAutoConsoleVariableRef CVARDebugPrintPlayerMovementComponentVelocity(
	TEXT("B.PrintPlayerMovementComponentVelocity"),
	PrintPlayerMovementComponentVelocity,
	TEXT("Print Player Movement Velocity Log"),
	ECVF_Cheat);

static int32 PrintPlayerMovementComponentRotation = 1;
FAutoConsoleVariableRef CVARDebugPrintPlayerMovementComponentRotation(
	TEXT("B.PrintPlayerMovementComponentRotation"),
	PrintPlayerMovementComponentRotation,
	TEXT("Print Player Movement Rotation Log"),
	ECVF_Cheat);


UBPlayerMovementComponent::UBPlayerMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	DefaultMass = 100;					// 100kg
	DefaultMovingForceScalar = 180000;	// 180000cN (㎏ × (cm/s^2))cN
	// Acceleration : 1800 (cm/s^2) => 1.8 (m/s^2)

	MinTurningRadius = 50;				// 50 cm

	// Person(upright position) : 1.0 - 1.3
	DragCoefficient = 1.3;

	// dry roads : 0.7 , wet roads : 0.4
	FrictionCoefficient = 0.7;
}

void UBPlayerMovementComponent::BeginPlay()
{
	Super::BeginPlay();


	AActor* Owner = GetOwner();

	// m / s^2으로 나오기 때문에 cm / s^2로 변환
	DefaultGravityScalar = FMath::Abs(Owner->GetWorld()->GetGravityZ());
	//DefaultMass = GetCharacterMovement()->Mass;

	DefaultAccelerationScalar = DefaultMovingForceScalar / DefaultMass;
	Velocity = FVector::ZeroVector;

	CurrentYaw = Owner->GetActorRotation().Yaw;
	// ControlRotation과 기준을 맞춘다.
	if (0.0f > CurrentYaw)
	{
		CurrentYaw += 360.0f;
	}
}

void UBPlayerMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ABPlayer* OwnerPlayer = Cast<ABPlayer>(GetOwner());
	if (!OwnerPlayer)
	{
		return;
	}

	const float ForwardMovementFactor = OwnerPlayer->GetForwardMovementFactor();
	const float RightMovementFactor = OwnerPlayer->GetRightMovementFactor();

	if (PrintPlayerMovementComponentVelocity)
	{
		B_LOG_DEV("%.1f, %.1f", ForwardMovementFactor, RightMovementFactor);
	}

	UpdateVelocity(ForwardMovementFactor, RightMovementFactor, DeltaTime);
	UpdateTransform(ForwardMovementFactor, RightMovementFactor, DeltaTime);
}

FVector UBPlayerMovementComponent::GetPlayerVelocity() const
{
	return Velocity;
}

float UBPlayerMovementComponent::GetCurrentYaw() const
{
	return CurrentYaw;
}

float UBPlayerMovementComponent::GetDeltaTranslationScalar(const FVector& CurrentVelocity, float DeltaTime) const
{
	return CurrentVelocity.Size() * DeltaTime;
}

float UBPlayerMovementComponent::GetAirResistanceScalar(const FVector& CurrentVelocity) const
{
	// 공기 저항 = (속도와 반대 방향) * 속도^2 * 항력계수
	return CurrentVelocity.SizeSquared() * DragCoefficient;
}

float UBPlayerMovementComponent::GetFrictionResistanceScalar(const FVector& CurrentVelocity) const
{
	// 마찰력 = (속도와 반대 방향) * 수직항력 * 마찰계수 // 수직항력의 경우 지면을 수평으로 간주하고 계산한다. (M * G)
	const float NormalForce = DefaultMass * DefaultGravityScalar;
	return CurrentVelocity.GetSafeNormal().Size() * FrictionCoefficient * NormalForce;
}

void UBPlayerMovementComponent::UpdateVelocity(float ForwardMovementFactor, float RightMovementFactor, float DeltaTime)
{
	ApplyResistanceToVelocity(DeltaTime);
	ApplyInputToVelocity(ForwardMovementFactor, RightMovementFactor, DeltaTime);

	if (PrintPlayerMovementComponentVelocity)
	{
		B_LOG_DEV("Final Velocity : %.1f, %.1f, %.1f", Velocity.X, Velocity.Y, Velocity.Z);
	}
}

void UBPlayerMovementComponent::ApplyResistanceToVelocity(float DeltaTime)
{
	/** 저항 계산 */
	const float VelocityScalar = Velocity.Size();
	if (1.0f <= VelocityScalar)
	{
		/*
		-Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
		float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
		float NormalForce = Mass * AccelerationDueToGravity;
		-Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
		*/

		// 공기 저항 = (속도와 반대 방향) * 속도^2 * 항력계수
		const float AirResistanceScalar = (VelocityScalar * VelocityScalar) * DragCoefficient;

		// 마찰력 = (속도와 반대 방향) * 수직항력 * 마찰계수 // 수직항력의 경우 지면을 수평으로 간주하고 계산한다. (M * G)
		const float NormalForce = DefaultMass * DefaultGravityScalar;
		const float FrictionResistanceScalar = FrictionCoefficient * NormalForce;

		/** 마찰력, 공기저항 계산 */
		const float CurrentResistanceScalar = AirResistanceScalar + FrictionResistanceScalar;
		// F = ma => a = F / m
		FVector ResistanceAcceleration = -Velocity.GetSafeNormal() * (CurrentResistanceScalar / DefaultMass);

		/*
		const FVector ResistanceDeltaVelocity = Velocity.GetSafeNormal() * (ResistanceAccelerationScalar * DeltaTime);
		Velocity -= ResistanceDeltaVelocity;

		if (PrintPlayerMovementComponentVelocity)
		{
			B_LOG_DEV("ResistanceAccelerationScalar : %.1f, %.1f, %.1f", ResistanceAccelerationScalar);
		}
		*/

		Velocity = Velocity + ResistanceAcceleration * DeltaTime;
	}
	else
	{
		Velocity = FVector::ZeroVector;
	}
}

void UBPlayerMovementComponent::ApplyInputToVelocity(float ForwardMovementFactor, float RightMovementFactor, float DeltaTime)
{
	/** 입력 계산 */
	if (!ForwardMovementFactor && !RightMovementFactor)
	{
		return;
	}

	FVector InputRelativeDirection(ForwardMovementFactor, RightMovementFactor, 0.0f);
	InputRelativeDirection.Normalize();

	//const FQuat InputRelativeDirectionRotation = InputRelativeDirection.ToOrientationQuat();
	//const FQuat VelocityRotation = Velocity.ToOrientationQuat();

	const FQuat ActorRotation = GetOwner()->GetActorQuat();
	const FVector InputWorldDirection = ActorRotation.RotateVector(InputRelativeDirection);
	FVector InputAcceleration = InputWorldDirection * DefaultAccelerationScalar;

	Velocity = Velocity + (InputAcceleration * DeltaTime);

	/*
	const FQuat InputWorldDirectionRotation = InputRelativeDirectionRotation * (-VelocityRotation);
	// DefaultAccelerationScalar = DefaultMovingForceScalar / DefaultMass
	*/
	if (PrintPlayerMovementComponentVelocity)
	{
		B_LOG_DEV("InputWorldDirection : %.1f, %.1f, %.1f", InputWorldDirection.X, InputWorldDirection.Y, InputWorldDirection.Z);
		B_LOG_DEV("InputAcceleration : %.1f, %.1f, %.1f", InputAcceleration.X, InputAcceleration.Y, InputAcceleration.Z);
	}
}

float UBPlayerMovementComponent::ConvertToControlRotationRange(float angle) const
{
	// 정밀도를 소수점 2자리까지 보장한다.
	static const int AngleFactor = 100;
	static const int MaxAngle = 360 * AngleFactor;

	int intAngle = angle * AngleFactor;
	intAngle %= MaxAngle;

	if (0.0f > angle)
	{
		intAngle += MaxAngle;
	}

	return ((float)intAngle / AngleFactor);
}

void UBPlayerMovementComponent::UpdateTransform(float ForwardMovementFactor, float RightMovementFactor, float DeltaTime)
{
	const float DeltaTranslationScalar = GetDeltaTranslationScalar(Velocity, DeltaTime);
	UpdateRotation(DeltaTranslationScalar);
	UpdateLocation(DeltaTranslationScalar);
}

void UBPlayerMovementComponent::UpdateRotation(float DeltaTranslationScalar)
{
	/**
	 * ActorRotation은 Quaternion으로 되어있고, Rotator로 변환할 때, Yaw는 Atan2함수를 사용하여 변환한다.
	 * 따라서 ActionRotation은 [-180, 180]까지의 범위를 가진다.
	 * ControlRotation은 [0, 360]의 범위를 가진다.
	 * CurrentYaw를 변수로 두고 [0, 360]의 같은 범위에서 계산한다. 
	 * 최종적으로 CurrentYaw를 통해 ActorRotation을 설정한다.
	 */

	ABPlayer* OwnerPlayer = Cast<ABPlayer>(GetOwner());

	//FRotator ActorRot = OwnerPlayer->GetActorRotation();

	//const float CurrentYaw = ActorRot.Yaw;

	const FRotator ControlRot = OwnerPlayer->GetControlRotation();
	float TargetYaw = ControlRot.Yaw;

	float RemainingYaw = TargetYaw - CurrentYaw;
	float YawFactor = (RemainingYaw < 0) ? -1 : 1;
	float RemainingYawPositive = FMath::Abs<float>(RemainingYaw);
	if (180.0f < RemainingYawPositive)
	{
		RemainingYawPositive = 360.0f - RemainingYawPositive;
		YawFactor *= -1;

		RemainingYaw = RemainingYawPositive * YawFactor;
	}

	if (0.1f < RemainingYawPositive)
	{
		// 각도(θ) * 반지름(r) = 호의 길이(l)
		float DeltaYawPositive = FMath::RadiansToDegrees<float>(DeltaTranslationScalar / MinTurningRadius);
		if (DeltaYawPositive > RemainingYawPositive)
		{
			DeltaYawPositive = RemainingYawPositive;
		}

		const float DeltaYaw = DeltaYawPositive * YawFactor;
		CurrentYaw += DeltaYaw;
		CurrentYaw = ConvertToControlRotationRange(CurrentYaw);

		OwnerPlayer->SetActorRotation(FRotator(0.0f, CurrentYaw, 0.0f));
		const FRotator DeltaRotation(0.0f, DeltaYaw, 0.0f);
		Velocity = DeltaRotation.RotateVector(Velocity);

		if (PrintPlayerMovementComponentRotation)
		{
			B_LOG_DEV("=============================================================");
			B_LOG_DEV("RemainingYaw : % .1f", RemainingYaw);
			B_LOG_DEV("RemainingYawPositive : % .1f", RemainingYawPositive);
			B_LOG_DEV("DeltaYaw : % .1f", DeltaYaw);
			B_LOG_DEV("YawFactor : % .1f", YawFactor);
			B_LOG_DEV("CurrentYaw : % .1f", CurrentYaw);
			B_LOG_DEV("TargetYaw : % .1f", TargetYaw);
			B_LOG_DEV("Velocity Applied Rotation : %.1f, %.1f, %.1f", Velocity.X, Velocity.Y, Velocity.Z);
		}
	}
	
}

void UBPlayerMovementComponent::UpdateLocation(float DeltaTranslationScalar)
{
	ABPlayer* OwnerPlayer = Cast<ABPlayer>(GetOwner());

	FHitResult Hit;
	OwnerPlayer->AddActorWorldOffset(Velocity.GetSafeNormal() * DeltaTranslationScalar, true, &Hit);

	/*
	if (Hit.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
	*/

	// 입력을 바로 반영할 때 사용
	/*
	const FRotator ActorRot = OwnerPlayer->GetActorRotation();

	FVector ForwardVector = ActorRot.Vector();
	FVector RightVector = FRotationMatrix(ActorRot).GetScaledAxis(EAxis::Y);

	FVector FinalDirection = ForwardVector * ForwardMovementFactor + RightVector * RightMovementFactor;
	FinalDirection.Normalize();
	*/
}

