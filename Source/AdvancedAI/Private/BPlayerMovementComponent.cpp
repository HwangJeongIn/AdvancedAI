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
	DefaultMovingForce = 10000; // 10000(㎏ × (cm/s^2))
	MinTurningRadius = 50;		// 50 cm

	DragCoefficient = 16;
	FrictionCoefficient = 0.015;
}

void UBPlayerMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// 시뮬레이션 테스트
	// 중력이 중간에 변한다면 실시간으로 계산해야한다.
	
	// m / s^2으로 나오기 때문에 cm / s^2로 변환
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
	* ActorRotation은 Quaternion으로 되어있고, Rotator로 변환할 때, Yaw는 Atan2함수를 사용하여 변환한다.
	* 따라서 ActionRotation은 [-180, 180]까지의 범위를 가진다.
	* ControlRotation은 [0, 360]의 범위를 가진다.
	*/

	ABPlayer* OwnerPlayer = Cast<ABPlayer>(GetOwner());

	FRotator ActorRot = OwnerPlayer->GetActorRotation();
	// 회전 업데이트
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
	// 공기 저항 = (속도와 반대 방향) * 속도^2 * 항력계수
	return CurrentVelocity.SizeSquared() * DragCoefficient;
}

float UBPlayerMovementComponent::GetFrictionResistanceScalar() const
{
	// 마찰력 = (속도와 반대 방향) * 수직항력 * 마찰계수 // 수직항력의 경우 지면을 수평으로 간주하고 계산한다. (M * G)
	const float NormalForce = DefaultMass * DefaultGravity;
	return FrictionCoefficient * NormalForce;
}

