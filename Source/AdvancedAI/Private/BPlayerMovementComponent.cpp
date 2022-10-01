// Fill out your copyright notice in the Description page of Project Settings.


#include "BPlayerMovementComponent.h"
#include "Engine/World.h"
#include "BPlayer.h"
#include "GameFramework/GameStateBase.h"

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
	PrimaryComponentTick.bCanEverTick = true;

	DefaultMass = 100;					// 100kg
	MaxVelocityScalar = 1000;			// 1000 cm/s = 10 m/s
	CurrentMaxVelocityFactor = 1.0f;
	MaxVelocityScalar = MaxVelocityScalar * CurrentMaxVelocityFactor;

	// MaxVelocity �������� ����Ѵ�.
	// MovementForceScalar = 100000;	// 100000cN (�� �� (cm/s^2))cN
	// Acceleration : 1000 (cm/s^2) => 10 (m/s^2)

	MinTurningRadius = 100;				// 100 cm

	// Person(upright position) : 1.0 - 1.3rr
	DragCoefficient = 1.3;

	// dry roads : 0.7 , wet roads : 0.4
	FrictionCoefficient = 0.7;
}

void UBPlayerMovementComponent::BeginPlay()
{
	Super::BeginPlay();


	AActor* Owner = GetOwner();

	// �ӵ� ������ cm�����ε�, �ӵ�^2�ϴ� ���� ������ ������ 100���� �����ش�. ��Ȯ�ϰ� Ȯ���� �ʿ䰡 �ִ�.
	DragCoefficient /= 100.0f;

	//FrictionCoefficient /= 100.0f;

	// cm / s^2 ������ ��ȯ
	DefaultGravityScalar = FMath::Abs(Owner->GetWorld()->GetGravityZ());
	//DefaultMass = GetCharacterMovement()->Mass;

	Velocity = FVector::ZeroVector;

	CurrentYaw = Owner->GetActorRotation().Yaw;
	// ControlRotation�� ������ �����.
	if (0.0f > CurrentYaw)
	{
		CurrentYaw += 360.0f;
	}

	RefreshMovementVariable();
}

void UBPlayerMovementComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);


	ABPlayer* OwnerPlayer = Cast<ABPlayer>(GetOwner());
	if (nullptr == OwnerPlayer)
	{
		return;
	}

	const float ForwardMovementFactor = OwnerPlayer->GetForwardMovementFactor();
	const float RightMovementFactor = OwnerPlayer->GetRightMovementFactor();

	// if (GetOwnerRole() == ROLE_AutonomousProxy || GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	// Ŭ���̾�Ʈ�� ������ ��Ʈ���ϰ� �ִ� ���� ���
	if (OwnerPlayer->IsLocallyControlled())
	{
		const AGameStateBase* CurrentGameState = nullptr != GetWorld() ? GetWorld()->GetGameState() : nullptr;
		if (nullptr == CurrentGameState)
		{
			return;
		}

		//LastMoveObject.SharedWorldTime = CurrentGameState->GetServerWorldTimeSeconds();
		//LastMoveObject.DeltaTime = DeltaTime;
		//LastMoveObject.ForwardMovementFactor = ForwardMovementFactor;
		//LastMoveObject.RightMovementFactor = RightMovementFactor;

		LastMoveObject.Set(CurrentGameState->GetServerWorldTimeSeconds()
						, DeltaTime
						, ForwardMovementFactor
						, RightMovementFactor);
		
		SimulateMoveObject(LastMoveObject);
	}
}

/*
bool UBPlayerMovementComponent::CreateMoveObject(float DeltaTime, FPlayerMoveObject& NewMoveObject) const
{
	const AGameStateBase* CurrentGameState = nullptr != GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (nullptr == CurrentGameState)
	{
		return false;
	}

	NewMoveObject.DeltaTime = DeltaTime;
	NewMoveObject.SharedTime = CurrentGameState->GetServerWorldTimeSeconds();

	NewMoveObject.SteeringThrow = SteeringThrow;
	NewMoveObject.Throttle = Throttle;
	return true;
}
*/

void UBPlayerMovementComponent::SetMaxVelocityFactor(const float NewMaxVelocityFactor)
{
	CurrentMaxVelocityFactor = NewMaxVelocityFactor;
	RefreshMovementVariable();
}

FVector UBPlayerMovementComponent::GetPlayerVelocity() const
{
	return Velocity;
}

float UBPlayerMovementComponent::GetCurrentYaw() const
{
	return CurrentYaw;
}


// Begin : Replication ���� �ڵ� =============================================================================================


void UBPlayerMovementComponent::SimulateMoveObject(const FPlayerMoveObject& MoveObject)
{
	const float DeltaTime = MoveObject.DeltaTime;
	const float ForwardMovementFactor = MoveObject.ForwardMovementFactor;
	const float RightMovementFactor = MoveObject.RightMovementFactor;

	if (PrintPlayerMovementComponentVelocity)
	{
		B_LOG_DEV("%.1f, %.1f", ForwardMovementFactor, RightMovementFactor);
	}

	UpdateVelocity(ForwardMovementFactor, RightMovementFactor, DeltaTime);
	UpdateTransform(ForwardMovementFactor, RightMovementFactor, DeltaTime);
}

// End : Replication ���� �ڵ� =============================================================================================

// Begin : Transform, Physics ���� �ڵ� ======================================================================================

void UBPlayerMovementComponent::RefreshMovementVariable()
{
	CurrentMaxVelocityScalar = MaxVelocityScalar * CurrentMaxVelocityFactor;

	// �ִ� �ӷ��� �� �����ϴ� ���� ���� ���� ����Ѵ�.
	MovementForceScalar = GetResistanceScalar(CurrentMaxVelocityScalar);
	AccelerationScalar = MovementForceScalar / DefaultMass;
}

float UBPlayerMovementComponent::GetDeltaTranslationScalar(const FVector& CurrentVelocity, float DeltaTime) const
{
	return CurrentVelocity.Size() * DeltaTime;
}

float UBPlayerMovementComponent::GetResistanceScalar(const float VelocityScalar /* Speed */) const
{
	// ���� ���� = (�ӵ��� �ݴ� ����) * �ӵ�^2 * �׷°��
	const float AirResistanceScalar = GetAirResistanceScalar(VelocityScalar);

	// ������ = (�ӵ��� �ݴ� ����) * �����׷� * ������� // �����׷��� ��� ������ �������� �����ϰ� ����Ѵ�. (M * G)
	const float FrictionResistanceScalar = GetFrictionResistanceScalar();

	/** ������, �������� ��� */
	const float CurrentResistanceScalar = AirResistanceScalar + FrictionResistanceScalar;
	return CurrentResistanceScalar;
}

float UBPlayerMovementComponent::GetAirResistanceScalar(const float VelocityScalar /* Speed */) const
{
	// ���� ���� = (�ӵ��� �ݴ� ����) * �ӵ�^2 * �׷°��
	return VelocityScalar * VelocityScalar * DragCoefficient;
}

float UBPlayerMovementComponent::GetFrictionResistanceScalar() const
{
	// ������ = (�ӵ��� �ݴ� ����) * �����׷� * ������� // �����׷��� ��� ������ �������� �����ϰ� ����Ѵ�. (M * G)
	const float NormalForce = DefaultMass * DefaultGravityScalar;
	return FrictionCoefficient * NormalForce;
}

void UBPlayerMovementComponent::UpdateVelocity(float ForwardMovementFactor, float RightMovementFactor, float DeltaTime)
{
	ApplyInputToVelocity(ForwardMovementFactor, RightMovementFactor, DeltaTime);
	ApplyResistanceToVelocity(DeltaTime);
}

void UBPlayerMovementComponent::ApplyResistanceToVelocity(float DeltaTime)
{
	/** ���� ��� */
	const float VelocityScalar = Velocity.Size();
	if (VelocityScalar < SMALL_NUMBER)
	{
		return;
	}

	const float ResistanceAccelerationScalar = (GetResistanceScalar(VelocityScalar) / DefaultMass);
	const float DeltaResistanceVelocityScalar = DeltaTime * ResistanceAccelerationScalar;

	if (VelocityScalar <= DeltaResistanceVelocityScalar)
	{ // ���� �Ŀ��� ���� �ۿ��ϸ� �ȵȴ�.
		Velocity = FVector::ZeroVector;
		return;
	}

	const FVector DeltaResistanceVelocity = -Velocity.GetSafeNormal() * DeltaResistanceVelocityScalar;
	if (PrintPlayerMovementComponentVelocity)
	{
		B_LOG_DEV("ApplyResistanceToVelocity=========================================================================================");
		B_LOG_DEV("PrevVelocity : %.1f, %.1f, %.1f", Velocity.X, Velocity.Y, Velocity.Z);
		B_LOG_DEV("DeltaResistanceVelocity : %.1f, %.1f, %.1f", DeltaResistanceVelocity.X, DeltaResistanceVelocity.Y, DeltaResistanceVelocity.Z);
		B_LOG_DEV("DeltaResistanceVelocityScalar : %.1f", DeltaResistanceVelocityScalar);

		B_LOG_DEV("VelocityScalar : %.1f", VelocityScalar);
		B_LOG_DEV("AirResistanceScalar : %.1f", GetAirResistanceScalar(VelocityScalar));
		B_LOG_DEV("FrictionResistanceScalar : %.1f", GetFrictionResistanceScalar());
	}

	Velocity = Velocity + DeltaResistanceVelocity;

	if (PrintPlayerMovementComponentVelocity)
	{
		B_LOG_DEV("CurrentVelocity : %.1f, %.1f, %.1f", Velocity.X, Velocity.Y, Velocity.Z);
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

	// AccelerationScalar = MovementForceScalar / DefaultMass;
	const FVector InputDeltaVelocity = InputWorldDirection * AccelerationScalar * DeltaTime;
	Velocity = Velocity + InputDeltaVelocity;

	/*
	const FQuat InputWorldDirectionRotation = InputRelativeDirectionRotation * (-VelocityRotation);
	// AccelerationScalar = MovementForceScalar / DefaultMass
	*/
	if (PrintPlayerMovementComponentVelocity)
	{
		B_LOG_DEV("ApplyInputToVelocity=========================================================================================");
		B_LOG_DEV("InputWorldDirection : %.1f, %.1f, %.1f", InputWorldDirection.X, InputWorldDirection.Y, InputWorldDirection.Z);
		B_LOG_DEV("InputDeltaVelocity : %.1f, %.1f, %.1f", InputDeltaVelocity.X, InputDeltaVelocity.Y, InputDeltaVelocity.Z);

		B_LOG_DEV("Final Velocity : %.1f, %.1f, %.1f", Velocity.X, Velocity.Y, Velocity.Z);
	}
}

float UBPlayerMovementComponent::ConvertToControlRotationRange(float angle) const
{
	// ���е��� �Ҽ��� 2�ڸ����� �����Ѵ�.
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
		// ����(��) * ������(r) = ȣ�� ����(l)
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

	// �Է��� �ٷ� �ݿ��� �� ���
	/*
	const FRotator ActorRot = OwnerPlayer->GetActorRotation();

	FVector ForwardVector = ActorRot.Vector();
	FVector RightVector = FRotationMatrix(ActorRot).GetScaledAxis(EAxis::Y);

	FVector FinalDirection = ForwardVector * ForwardMovementFactor + RightVector * RightMovementFactor;
	FinalDirection.Normalize();
	*/
}

// End : Transform, Physics ���� �ڵ� ======================================================================================
