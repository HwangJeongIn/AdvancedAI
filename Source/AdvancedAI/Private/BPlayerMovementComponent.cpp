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


	Velocity = FVector::ZeroVector;

	DefaultMass = 100;			// 100kg
	DefaultMovingForce = 10000; // 10000(�� �� (cm/s^2))
	MinTurningRadius = 50;		// 50 cm

	DragCoefficient = 16;
	FrictionCoefficient = 0.015;
}

void UBPlayerMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// �ùķ��̼� �׽�Ʈ
	// �߷��� �߰��� ���Ѵٸ� �ǽð����� ����ؾ��Ѵ�.
	
	// m / s^2���� ������ ������ cm / s^2�� ��ȯ
	DefaultGravity = GetOwner()->GetWorld()->GetGravityZ() * 100;
	//DefaultMass = GetCharacterMovement()->Mass;
}

void UBPlayerMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ABPlayer* OwnerPlayer = Cast<ABPlayer>(GetOwner());
	if (!OwnerPlayer)
	{
		return;
	}

	const FVector PlayerForwardDirection = ControlRot.Vector();

	const float ForwardMovementFactor = OwnerPlayer->GetForwardMovementFactor();
	const float RightMovementFactor = OwnerPlayer->GetRightMovementFactor();

	float CurrentForceScalar = DefaultMovingForce * ForwardMovementFactor;
	const float CurrentResistanceScalar = GetAirResistanceScalar(Velocity) + GetFrictionResistanceScalar();

	CurrentForceScalar -= CurrentResistanceScalar;

	// F = ma => a = F / m
	FVector Acceleration = PlayerForwardDirection * CurrentForceScalar / DefaultMass;
	Velocity = Velocity + Acceleration * DeltaTime;

	UpdateTransform(ForwardMovementFactor, RightMovementFactor, DeltaTime);

	
	/*
	UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(RootComponent);
	if (RootComp)
	{
		B_LOG_DEV("%.1f, %.1f, %.1f", FinalVelocity.X, FinalVelocity.Y, FinalVelocity.Z);
		RootComp->SetPhysicsLinearVelocity(FinalVelocity);
	}

	//B_LOG_DEV("%.1f, %.1f, %.1f", finalMovingForce.X, finalMovingForce.Y, finalMovingForce.Z);
	//GetCharacterMovement()->AddForce(finalMovingForce);
	*/
	if (!ForwardMovementFactor && !RightMovementFactor)
	{
		return;
	}

	//B_LOG_DEV("%.1f, %.1f, %.1f", CurrentVelocity.X, CurrentVelocity.Y, CurrentVelocity.Z);
	//B_LOG_DEV("Test : %.1f", RotationAngle);


}

void UBPlayerMovementComponent::UpdateTransform(float ForwardMovementFactor, float RightMovementFactor, float DeltaTime)
{
	const float DeltaTranslationScalar = GetDeltaTranslationScalar(Velocity, DeltaTime);
	UpdateRotation(DeltaTranslationScalar);
	UpdateLocation(ForwardMovementFactor, RightMovementFactor, DeltaTime);
}


void UBPlayerMovementComponent::UpdateLocation(float ForwardMovementFactor, float RightMovementFactor, float DeltaTranslationScalar)
{
	ABPlayer* OwnerPlayer = Cast<ABPlayer>(GetOwner());

	const FRotator ActorRot = OwnerPlayer->GetActorRotation();
	FHitResult Hit;

	FVector ForwardVector = ActorRot.Vector();
	FVector RightVector = FRotationMatrix(ActorRot).GetScaledAxis(EAxis::Y);

	FVector FinalDirection = ForwardVector * ForwardMovementFactor + RightVector * RightMovementFactor;
	FinalDirection.Normalize();
	OwnerPlayer->AddActorWorldOffset(FinalDirection * DeltaTranslationScalar, true, &Hit);

	/*
	if (Hit.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}
	*/
}

void UBPlayerMovementComponent::UpdateRotation(float DeltaTranslationScalar)
{
	/*
	* ActorRotation�� Quaternion���� �Ǿ��ְ�, Rotator�� ��ȯ�� ��, Yaw�� Atan2�Լ��� ����Ͽ� ��ȯ�Ѵ�.
	* ���� ActionRotation�� [-180, 180]������ ������ ������.
	* ControlRotation�� [0, 360]�� ������ ������.
	*/

	ABPlayer* OwnerPlayer = Cast<ABPlayer>(GetOwner());

	FRotator ActorRot = OwnerPlayer->GetActorRotation();
	// ȸ�� ������Ʈ
	{
		const float CurrentYaw = ActorRot.Yaw;

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
				B_LOG_DEV("ControlRot.Yaw : %.1f", ControlRot.Yaw);
				B_LOG_DEV("ActorRot.Yaw : % .1f", ActorRot.Yaw);
				//B_LOG_DEV("ActorYaw : % .1f", ActorYaw);
				B_LOG_DEV("RemainingAngle : %.1f", RemainingYaw);
				B_LOG_DEV("DeltaYaw : %.1f", DeltaYaw);
				//B_LOG_DEV("Ratio : %.1f", Ratio);
			}
		}
	}
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

float UBPlayerMovementComponent::GetFrictionResistanceScalar() const
{
	// ������ = (�ӵ��� �ݴ� ����) * �����׷� * ������� // �����׷��� ��� ������ �������� �����ϰ� ����Ѵ�. (M * G)
	const float NormalForce = DefaultMass * DefaultGravity;
	return FrictionCoefficient * NormalForce;
}

