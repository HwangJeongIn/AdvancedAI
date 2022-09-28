// Fill out your copyright notice in the Description page of Project Settings.


#include "BPlayerMovementComponent.h"
#include "Engine/World.h"
#include "BPlayer.h"	

static int32 PrintPlayerMovementComponent = 0;
FAutoConsoleVariableRef CVARDebugPrintPlayerMovementComponent(
	TEXT("B.PrintPlayerMovementComponent"),
	PrintPlayerMovementComponent,
	TEXT("Print Player Movement Log"),
	ECVF_Cheat);


UBPlayerMovementComponent::UBPlayerMovementComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;



	DefaultMass = 100;					// 100kg
	DefaultMovingForceScalar = 100000;	// 100000(㎏ × (cm/s^2))cN
	MinTurningRadius = 50;				// 50 cm

	DragCoefficient = 16;
	FrictionCoefficient = 0.015;
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

	if (PrintPlayerMovementComponent)
	{
		B_LOG_DEV("%.1f, %.1f", ForwardMovementFactor, RightMovementFactor);
	}

	UpdateVelocity(ForwardMovementFactor, RightMovementFactor, DeltaTime);
	UpdateTransform(ForwardMovementFactor, RightMovementFactor, DeltaTime);
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

	if (PrintPlayerMovementComponent)
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

		if (PrintPlayerMovementComponent)
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
	if (PrintPlayerMovementComponent)
	{
		B_LOG_DEV("InputWorldDirection : %.1f, %.1f, %.1f", InputWorldDirection.X, InputWorldDirection.Y, InputWorldDirection.Z);
		B_LOG_DEV("InputAcceleration : %.1f, %.1f, %.1f", InputAcceleration.X, InputAcceleration.Y, InputAcceleration.Z);
	}
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
	if (180.0f < TargetYaw)
	{
		TargetYaw = TargetYaw - 360.0f;
	}

	float RemainingYaw = TargetYaw - CurrentYaw;
	float YawFactor = (RemainingYaw < 0) ? -1 : 1;
	float RemainingYawPositive = FMath::Abs<float>(RemainingYaw);
	if (180.0f < RemainingYawPositive)
	{
		RemainingYawPositive = 360.0f - RemainingYawPositive;
	}

	if (0.1f < RemainingYawPositive)
	{
		RemainingYaw = RemainingYawPositive * (-YawFactor);

		// 각도(θ) * 반지름(r) = 호의 길이(l)
		float DeltaYaw = DeltaTranslationScalar / MinTurningRadius; // *RightMovementFactor;
		DeltaYaw = FMath::RadiansToDegrees<float>(DeltaYaw) * (-YawFactor);
		const FRotator DeltaRotation(0.0f, DeltaYaw, 0.0f);

		ActorRot += DeltaRotation;
		ActorRot.Pitch = 0.0f;
		ActorRot.Roll = 0.0f;
		OwnerPlayer->SetActorRotation(ActorRot);

		Velocity = DeltaRotation.RotateVector(Velocity);

		if (PrintPlayerMovementComponent)
		{
			B_LOG_DEV("=============================================================");
			B_LOG_DEV("Velocity Applied Rotation : %.1f, %.1f, %.1f", Velocity.X, Velocity.Y, Velocity.Z);
			B_LOG_DEV("ControlRot.Yaw : %.1f", ControlRot.Yaw);
			B_LOG_DEV("ActorRot.Yaw : % .1f", ActorRot.Yaw);
			//B_LOG_DEV("ActorYaw : % .1f", ActorYaw);
			B_LOG_DEV("RemainingAngle : %.1f", RemainingYaw);
			B_LOG_DEV("DeltaYaw : %.1f", DeltaYaw);
			//B_LOG_DEV("Ratio : %.1f", Ratio);
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

