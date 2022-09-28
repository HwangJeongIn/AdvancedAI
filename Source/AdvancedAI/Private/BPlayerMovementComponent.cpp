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
	DefaultMovingForceScalar = 100000;	// 100000(�� �� (cm/s^2))cN
	MinTurningRadius = 50;				// 50 cm

	DragCoefficient = 16;
	FrictionCoefficient = 0.015;
}

void UBPlayerMovementComponent::BeginPlay()
{
	Super::BeginPlay();


	AActor* Owner = GetOwner();

	// m / s^2���� ������ ������ cm / s^2�� ��ȯ
	DefaultGravityScalar = FMath::Abs(Owner->GetWorld()->GetGravityZ());
	//DefaultMass = GetCharacterMovement()->Mass;

	DefaultAccelerationScalar = DefaultMovingForceScalar / DefaultMass;
	Velocity = FVector::ZeroVector;

	CurrentYaw = Owner->GetActorRotation().Yaw;
	// ControlRotation�� ������ �����.
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
	// ���� ���� = (�ӵ��� �ݴ� ����) * �ӵ�^2 * �׷°��
	return CurrentVelocity.SizeSquared() * DragCoefficient;
}

float UBPlayerMovementComponent::GetFrictionResistanceScalar(const FVector& CurrentVelocity) const
{
	// ������ = (�ӵ��� �ݴ� ����) * �����׷� * ������� // �����׷��� ��� ������ �������� �����ϰ� ����Ѵ�. (M * G)
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
	/** ���� ��� */
	const float VelocityScalar = Velocity.Size();
	if (1.0f <= VelocityScalar)
	{
		/*
		-Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
		float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100;
		float NormalForce = Mass * AccelerationDueToGravity;
		-Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
		*/

		// ���� ���� = (�ӵ��� �ݴ� ����) * �ӵ�^2 * �׷°��
		const float AirResistanceScalar = (VelocityScalar * VelocityScalar) * DragCoefficient;

		// ������ = (�ӵ��� �ݴ� ����) * �����׷� * ������� // �����׷��� ��� ������ �������� �����ϰ� ����Ѵ�. (M * G)
		const float NormalForce = DefaultMass * DefaultGravityScalar;
		const float FrictionResistanceScalar = FrictionCoefficient * NormalForce;

		/** ������, �������� ��� */
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
	/** �Է� ��� */
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
	 * ActorRotation�� Quaternion���� �Ǿ��ְ�, Rotator�� ��ȯ�� ��, Yaw�� Atan2�Լ��� ����Ͽ� ��ȯ�Ѵ�.
	 * ���� ActionRotation�� [-180, 180]������ ������ ������.
	 * ControlRotation�� [0, 360]�� ������ ������.
	 * CurrentYaw�� ������ �ΰ� [0, 360]�� ���� �������� ����Ѵ�. 
	 * ���������� CurrentYaw�� ���� ActorRotation�� �����Ѵ�.
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

		// ����(��) * ������(r) = ȣ�� ����(l)
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

	// �Է��� �ٷ� �ݿ��� �� ���
	/*
	const FRotator ActorRot = OwnerPlayer->GetActorRotation();

	FVector ForwardVector = ActorRot.Vector();
	FVector RightVector = FRotationMatrix(ActorRot).GetScaledAxis(EAxis::Y);

	FVector FinalDirection = ForwardVector * ForwardMovementFactor + RightVector * RightMovementFactor;
	FinalDirection.Normalize();
	*/
}

