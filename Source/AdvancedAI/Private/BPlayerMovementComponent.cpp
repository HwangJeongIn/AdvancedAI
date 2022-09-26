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

	FRotator ControlRot = OwnerPlayer->GetControlRotation();
	ControlRot.Pitch = 0.0f;
	ControlRot.Roll = 0.0f;

	const FVector PlayerForwardDirection = ControlRot.Vector();

	const float MovingFactor = OwnerPlayer->GetMovingFactor();
	const float RotationFactor = OwnerPlayer->GetRotationFactor();


	float CurrentForceScalar = DefaultMovingForce * MovingFactor;
	const float CurrentResistanceScalar = GetAirResistanceScalar(Velocity) + GetFrictionResistanceScalar();

	CurrentForceScalar -= CurrentResistanceScalar;

	// F = ma => a = F / m
	FVector Acceleration = PlayerForwardDirection * CurrentForceScalar / DefaultMass;
	Velocity = Velocity + Acceleration * DeltaTime;


	
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
	if (!MovingFactor && !RotationFactor)
	{
		return;
	}

	//B_LOG_DEV("%.1f, %.1f, %.1f", CurrentVelocity.X, CurrentVelocity.Y, CurrentVelocity.Z);
	//B_LOG_DEV("Test : %.1f", RotationAngle);


}

void UBPlayerMovementComponent::UpdateRotation()
{
	const float DeltaTime = 3.0f;


	//=================================================================================

	ABPlayer* OwnerPlayer = Cast<ABPlayer>(GetOwner());
	/*
		ActorRotation�� Quaternion���� �Ǿ��ְ�, Rotator�� ��ȯ�� ��, Yaw�� Atan2�Լ��� ����Ͽ� ��ȯ�Ѵ�.
		���� ActionRotation�� [-180, 180]������ ������ ������.

		ControlRotation�� [0, 360]�� ������ ������.
	*/

	const FRotator ControlRot = OwnerPlayer->GetControlRotation();
	float TargetYaw = ControlRot.Yaw;
	if (180.0f < TargetYaw)
	{
		TargetYaw = TargetYaw - 360.0f;
	}

	FRotator ActorRot = OwnerPlayer->GetActorRotation();
	float  = ActorRot.Yaw;
	if (0 > ActorYaw)
	{
		ActorYaw = 360.0f + ActorYaw;
	}

	// �� ȸ������ Yaw�� ���̰� ������ �������ش�. ���� ��, �ӵ��� ȸ���ݰ����� ����Ѵ�.
	const float RemainingYaw = ControlRot.Yaw - ActorYaw;
	float RemainingYawPositive = FMath::Abs<float>(RemainingYaw);
	if (180.0f < RemainingYawPositive)
	{
		RemainingYawPositive = 360.0f - RemainingYawPositive;
	}

	if (0.1f > RemainingYawPositive)
	{
		return;
	}

	const float DeltaTranslationScalar = Velocity.Size() * DeltaTime;
	// ����(��) * ������(r) = ȣ�� ����(l)
	float DeltaYaw = DeltaTranslationScalar / MinTurningRadius;// *RotationFactor;
	DeltaYaw = FMath::RadiansToDegrees<float>(DeltaYaw);

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
	OwnerPlayer->SetActorRotation(NewRotator);

	AddMovementInput(NewRotator.Vector(), MovingFactor);

	// ��ġ ������Ʈ
	const FVector DeltaTranslation = Velocity * DeltaTime;
	FHitResult Hit;
	OwnerPlayer->AddActorWorldOffset(DeltaTranslation, true, &Hit);

	if (Hit.IsValidBlockingHit())
	{
		Velocity = FVector::ZeroVector;
	}

	if (PrintPlayerMovementComponent)
	{
		B_LOG_DEV("=============================================================");
		B_LOG_DEV("ControlRot.Yaw : %.1f", ControlRot.Yaw);
		B_LOG_DEV("ActorRot.Yaw : % .1f", ActorRot.Yaw);
		B_LOG_DEV("ActorYaw : % .1f", ActorYaw);
		B_LOG_DEV("RemainingAngle : %.1f", RemainingYaw);
		B_LOG_DEV("DeltaYaw : %.1f", DeltaYaw);
		B_LOG_DEV("Ratio : %.1f", Ratio);
	}
}

// �ùķ��̼� �׽�Ʈ
float UBPlayerMovementComponent::GetAirResistanceScalar(const FVector& CurrentVelocity)
{	
	// ���� ���� = (�ӵ��� �ݴ� ����) * �ӵ�^2 * �׷°��
	return CurrentVelocity.SizeSquared() * DragCoefficient;
}

float UBPlayerMovementComponent::GetFrictionResistanceScalar()
{
	// ������ = (�ӵ��� �ݴ� ����) * �����׷� * ������� // �����׷��� ��� ������ �������� �����ϰ� ����Ѵ�. (M * G)
	const float NormalForce = DefaultMass * DefaultGravity;
	return FrictionCoefficient * NormalForce;
}

