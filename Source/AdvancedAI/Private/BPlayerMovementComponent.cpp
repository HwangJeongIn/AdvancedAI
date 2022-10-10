// Fill out your copyright notice in the Description page of Project Settings.


#include "BPlayerMovementComponent.h"
#include "Engine/World.h"
#include "BPlayer.h"
#include "BPlayerController.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"


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

static int32 PrintPlayerMovementComponentClientInterpolation = 1;
FAutoConsoleVariableRef CVARDebugPrintPlayerMovementComponentClientInterpolation(
	TEXT("B.PrintPlayerMovementComponentClientInterpolation"),
	PrintPlayerMovementComponentClientInterpolation,
	TEXT("Print Player Movement Client Interpolation Log"),
	ECVF_Cheat);



static int32 PrintPlayerMovementComponentReplication = 1;
FAutoConsoleVariableRef CVARDebugPrintPlayerMovementComponentReplication(
	TEXT("B.PrintPlayerMovementComponentReplication"),
	PrintPlayerMovementComponentReplication,
	TEXT("Print Player Movement Replication Log"),
	ECVF_Cheat);


UBPlayerMovementComponent::UBPlayerMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicated(true);

	DefaultMass = 100;					// 100kg
	MaxVelocityScalar = 1000;			// 1000 cm/s = 10 m/s
	CurrentMaxVelocityFactor = 1.0f;
	MaxVelocityScalar = MaxVelocityScalar * CurrentMaxVelocityFactor;

	// MaxVelocity �������� ����Ѵ�.
	// MovementForceScalar = 100000;	// 100000cN (�� �� (cm/s^2))cN
	// Acceleration : 1000 (cm/s^2) => 10 (m/s^2)

	MinTurningRadius = 250;				// 250 cm

	// Person(upright position) : 1.0 - 1.3rr
	DragCoefficient = 1.3;

	// dry roads : 0.7 , wet roads : 0.4
	FrictionCoefficient = 0.7;

	InterpolationCompletionTime = 0.0f;
	CurrentInterpolationTime = 0.0f;

	//InterpolationRatio = 1.0f;
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
	const FVector ActorLocation = Owner->GetActorLocation();
	CubicSpline.Set(ActorLocation, FVector::ZeroVector, ActorLocation, FVector::ZeroVector);
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
	const ENetRole CurrentActorRole = OwnerPlayer->GetLocalRole();
	//const ENetRole CurrentActorRemoteRole = OwnerPlayer->GetRemoteRole();
	
	// Ŭ���̾�Ʈ�� ������ ��Ʈ���ϰ� �ִ� ���� ���
	if (OwnerPlayer->IsLocallyControlled())
	{
		float SharedWorldTime = 0.0f;
		if (false == GetSharedWorldTime(SharedWorldTime))
		{
			return;
		}

		FPlayerMovementObject LastMovementObject;
		LastMovementObject.Set(SharedWorldTime
						, DeltaTime
						, ForwardMovementFactor
						, RightMovementFactor
						, OwnerPlayer->GetControlRotation());
		

		/**
		 * Ŭ���̾�Ʈ���� ��Ʈ�ѵǰ� �ִ� ���, ���� �Է��� Ŭ���̾�Ʈ���� �ùķ��̼��ϰ�, �Է��� ������ �����ְ� �Է� ����Ʈ�� �����Ѵ�.
		 * ���Ŀ� Ŭ���̾�Ʈ�� �Է��� ���������� �ùķ��̼ǵǰ�, MovementState�� ���ø����̼ǵȴ�.
		 *
		 * �������� ��Ʈ�ѵǰ� �ִ� ��� �ٷ� �ùķ��̼��� �����ϰ�, MovementState�� ���ø����̼��Ѵ�.
		 * (�տ��� ���� �ý��ۿ��� ��Ʈ�ѵȴٴ� ���� üũ�߱� ������ Role�� ROLE_Authority���� Ȯ���ϸ� �������� ��Ʈ�ѵǰ� �ִ� ������ �� �� �ִ�.)
		*/

		// ������ Ŭ���̾�Ʈ�� �ϴ� ���ÿ��� �ùķ��̼��Ѵ�.
		SimulateMovementObject(LastMovementObject);

		switch (CurrentActorRole)
		{
		case ROLE_AutonomousProxy:
		{
			PendingMovementObjects.Add(LastMovementObject);
			ServerMove(LastMovementObject);
		}
		break;
		case ROLE_Authority:
		{
			UpdateMovementState(LastMovementObject);
		}
		break;
		}

	}
	else if(CurrentActorRole == ROLE_SimulatedProxy) // �ùķ��̼� ���� ���̶�� �������� ���� MovementState�� �����Ͽ� ������ �����Ѵ�.
	{
		InterpolateFromClient(DeltaTime);
	}
}

void UBPlayerMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UBPlayerMovementComponent, MovementState);
}

void UBPlayerMovementComponent::SetMaxVelocityFactor(const float NewMaxVelocityFactor)
{
	CurrentMaxVelocityFactor = NewMaxVelocityFactor;
	RefreshMovementVariable();
}

FVector UBPlayerMovementComponent::GetVelocity() const
{
	return Velocity;
}

float UBPlayerMovementComponent::GetCurrentYaw() const
{
	return CurrentYaw;
	// [-180, 180 ���� ��ȯ]
	/*
	if (180.0f < CurrentYaw)
	{
		return 360.0f - CurrentYaw;
	}
	else
	{
		return CurrentYaw;
	}
	*/
}

float UBPlayerMovementComponent::GetMaxSpeed() const
{
	return MaxVelocityScalar;
}


// Begin : Replication ���� �ڵ� =============================================================================================

bool UBPlayerMovementComponent::GetSharedWorldTime(float& SharedWorldTime) const
{
	const AGameStateBase* CurrentGameState = nullptr != GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (nullptr == CurrentGameState)
	{
		B_ASSERT_DEV(false, "GameState �̳� World �� �������� �ʽ��ϴ�.")
			return false;
	}

	SharedWorldTime = CurrentGameState->GetServerWorldTimeSeconds();
	return true;
}

void UBPlayerMovementComponent::ServerMove_Implementation(FPlayerMovementObject MovementObject)
{
	if (PrintPlayerMovementComponentClientInterpolation)
	{
		//B_LOG_DEV("ServerMove_Implementation=============================================================");
		//MovementObject.Print();
	}

	ClientSimulatedTimeInServer += MovementObject.DeltaTime;

	SimulateMovementObject(MovementObject);
	UpdateMovementState(MovementObject);
}

bool UBPlayerMovementComponent::ServerMove_Validate(FPlayerMovementObject MovementObject)
{
	if (false == MovementObject.IsValid())
	{
		B_ASSERT_DEV(false, "Ŭ���̾�Ʈ Ű���� �Է� ���� �ùٸ��� �ʽ��ϴ�. ���� �α׾ƿ� ó���մϴ�.")
		return false;
	}

	float ServerSimulatedTime = 0.0f;
	if (false == GetSharedWorldTime(ServerSimulatedTime))
	{
		B_ASSERT_DEV(false, "�������� �ùķ��̼� �ð��� �޾ƿ� �� �����ϴ�. �ϴ� ���� �α׾ƿ� ó���մϴ�.")
		return false;
	}

	const float CurrentClientSimulatedTimeInServer = ClientSimulatedTimeInServer + MovementObject.DeltaTime;
	if (ServerSimulatedTime < CurrentClientSimulatedTimeInServer)
	{
		B_ASSERT_DEV(false, "Ŭ���̾�Ʈ�� �ùķ��̼� ���� �ð��� �������Դϴ�. ���� �α׾ƿ� ó���մϴ�.")
		return false;
	}

	return true;
}

void UBPlayerMovementComponent::UpdateMovementState(const FPlayerMovementObject& MovementObject)
{
	if (PrintPlayerMovementComponentClientInterpolation)
	{
		//B_LOG_DEV("UpdateMovementState=============================================================");
	}

	MovementState.LastMovementObjectSharedWorldTime = MovementObject.SharedWorldTime;
	MovementState.Tranform = GetOwner()->GetActorTransform();
	MovementState.Velocity = GetVelocity();
	/* const bool Result = */GetSharedWorldTime(MovementState.DepartureTime);
}


void UBPlayerMovementComponent::OnRep_MovementState()
{
	if (PrintPlayerMovementComponentClientInterpolation)
	{
		B_LOG_DEV("OnRep_MovementState=============================================================");
	}

	/**
	 * TODO : ������ Ŭ���̾�Ʈ�� ����ȭ�� �ð� ������ �������� �󸶳� ���� Ŭ���̾�Ʈ�� ���ø����̼� �Ǵ� �� Ȯ���ؾ� �Ѵ�.
	 * ����ȭ�� �� ���� �ʴ´ٸ� RTT / 2 �� ����� ��, �ٸ� ����� ����ؾ��Ѵ�.
	 *
	 * �ٸ� ���
	 *  : Ŭ���̾�Ʈ���� ��Ŷ�� ���� �� Ÿ�� �������� ��� ������, �������� �ٽ� Ŭ���̾�Ʈ�� ���� �� �ش� Ÿ�� �������� �״�� ������.
	 *  : Ŭ���̾�Ʈ���� �ش� Ÿ�� �������� �������� RTT�� ���ϰ� RTT / 2 �� ���Ѵ� */

	 /** RTT / 2 */
	float FromServerToClientTime = 0.0f;
	ABPlayer* CurrentPlayer = Cast<ABPlayer>(GetOwner());
	if (CurrentPlayer)
	{
		ABPlayerController* CurrentPlayerController = Cast<ABPlayerController>(CurrentPlayer->GetController());
		if (CurrentPlayerController)
		{
			FromServerToClientTime = CurrentPlayerController->GetRoundTripTimeHalf();
		}
	}

	const ENetRole CurrentActorRole = GetOwnerRole();
	switch (CurrentActorRole)
	{
	case ROLE_SimulatedProxy:
	{
		UpdateSimulatedProxyFromMovementState(FromServerToClientTime);
	}
	break;
	case ROLE_AutonomousProxy:
	{
		UpdateAutonomousProxyFromMovementState(FromServerToClientTime);
	}
	break;
	}
}



void UBPlayerMovementComponent::UpdateSimulatedProxyFromMovementState(float FromServerToClientTime /* RTT / 2 */)
{
	if (PrintPlayerMovementComponentClientInterpolation)
	{
		B_LOG_DEV("UpdateSimulatedProxyFromMovementState=============================================================");
	}
	/*
	 * SimulatedProxy �� ��� �������� ���� MovementState�� ������ ���̶�� �����Ͽ� �ùķ��̼��� �����Ѵ�.
	 * ���� RTT / 2 ��ŭ ���̳��� ������ MovementState�� ������� RTT / 2 ��ŭ �ùķ��̼����ش�.
	 * SimulatedProxy�� ��� �ùķ��̼� ������ ���� �����ϱ� ������ �������� ���� ���¿� ���� ���°� �������� ���� ���ɼ��� ����.
	 * ���ϴ� ���¿� �������� ���� �� �� ���̰� ũ�ٸ� �׳� �ڸ���Ʈ�Ѵ�.
	 * ������ ��쿡�� ���� ���ö��� �(���� ������Ʈ ���ö���)�� ���� ���� ���¸� �޴� ������ Ŭ���̾�Ʈ�� ������ ���·� �����Ѵ�.
	 */

	AActor* Owner = GetOwner();

	FVector CurrentVelocity = GetVelocity();
	FVector CurrentLocation = GetOwner()->GetActorLocation();

	// ������ ������ �����ϴ� ��ǥ ���°� ���� ���¿��� ��ġ�� 100cm �̻� ���̳���, ���� ��ǥ ���·� ������ �����Ѵ�.
	if (10000.0f < (CurrentLocation - CubicSpline.TargetLocation).SizeSquared())
	{
		SetTargetMovementBySpline(CubicSpline);
		Owner->SetActorRotation(TargetRotation);
	}

	StartRotation = Owner->GetActorQuat();
	TargetRotation = MovementState.Tranform.GetRotation();
	FVector TargetVelocity = MovementState.Velocity;
	FVector TargetLocation = MovementState.Tranform.GetLocation();

	// RTT / 2 �� ������� �ʰ� ������ �ֱ�� �̷��� �����ؼ� �ùķ��̼��Ѵ�.
	// ���� ���´� ���� ��Ŷ�� �����ϱ���� �ɸ� �ð��� ����ϰ� �ɸ��ٰ� �����Ѵ�.
	CalculateMovementByDeadReckoning(/*FromServerToClientTime +*/ CurrentInterpolationTime,
		TargetLocation, TargetVelocity);

	CubicSpline.Set(CurrentLocation, CurrentVelocity,
		TargetLocation, TargetVelocity);

	InterpolationCompletionTime = CurrentInterpolationTime;
	CurrentInterpolationTime = 0.0f;
	//InterpolationRatio = 0.0f;

	if (PrintPlayerMovementComponentClientInterpolation)
	{
		B_LOG_DEV("InterpolationCompletionTime : %.6f", InterpolationCompletionTime);
		B_LOG_DEV("StartLocation : %.1f, %.1f, %.1f", CubicSpline.StartLocation.X, CubicSpline.StartLocation.Y, CubicSpline.StartLocation.Z);
		B_LOG_DEV("TargetLocation : %.1f, %.1f, %.1f", CubicSpline.TargetLocation.X, CubicSpline.TargetLocation.Y, CubicSpline.TargetLocation.Z);

		B_LOG_DEV("StartVelocity : %.1f, %.1f, %.1f", CubicSpline.StartVelocity.X, CubicSpline.StartVelocity.Y, CubicSpline.StartVelocity.Z);
		B_LOG_DEV("TargetVelocity : %.1f, %.1f, %.1f", CubicSpline.TargetVelocity.X, CubicSpline.TargetVelocity.Y, CubicSpline.TargetVelocity.Z);

		FVector SplineTempLocation;
		for (float i = 0; i < 10; ++i)
		{
			SplineTempLocation = CubicSpline.InterpolateLocation(0.1 * i);

			DrawDebugSphere(GetWorld(), SplineTempLocation, 20, 12, FColor::Red, false, 2.0f, 0, 1.0f);
		}
	}
}

void UBPlayerMovementComponent::SetTargetMovementBySpline(FHermiteCubicSpline& Spline)
{
	GetOwner()->SetActorLocation(Spline.TargetLocation);
	SetVelocity(Spline.TargetVelocity);
}

void UBPlayerMovementComponent::CalculateMovementByDeadReckoning(float ElapsedTime,
	FVector& CurrentLocation, FVector& CurrentVelocity) const
{
	CurrentLocation += CurrentVelocity * ElapsedTime;

	// �� ����ؾ��ϸ� �߰�
}

void UBPlayerMovementComponent::InterpolateFromClient(float DeltaTime)
{
	//B_LOG_DEV("InterpolateFromClient => DeltaTime : %.1f", DeltaTime);

	CurrentInterpolationTime += DeltaTime;

	AActor* OwnerActor = GetOwner();
	if (0.0f < InterpolationCompletionTime) // �������� ����
	{
		const float InterpolationRatio = FMath::Clamp(CurrentInterpolationTime / InterpolationCompletionTime, 0.0f, 1.0f);
		if (1.0f == InterpolationRatio)
		{
			InterpolationCompletionTime = 0.0f;
		}

		// ��ġ, �ӵ� ���ö��� ����
		OwnerActor->SetActorLocation(CubicSpline.InterpolateLocation(InterpolationRatio));
		SetVelocity(CubicSpline.InterpolateVelocity(InterpolationRatio));

		// ȸ�� ���� ����
		const FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, InterpolationRatio);
		OwnerActor->SetActorRotation(NewRotation);

		if (PrintPlayerMovementComponentClientInterpolation)
		{
			B_LOG_DEV("Interpolating=========================================================================================");
			B_LOG_DEV("InterpolationRatio : %.1f", InterpolationRatio);
		}
	}
	else // �����Ϸ� �����̱� ������ ���� �ӵ� ������� �ùķ��̼��Ѵ�.
	{
		FVector CurrentActorLocation = OwnerActor->GetActorLocation();
		FVector CurrentVelocity = GetVelocity();

		CalculateMovementByDeadReckoning(DeltaTime, CurrentActorLocation, CurrentVelocity);

		OwnerActor->SetActorLocation(CurrentActorLocation);
		SetVelocity(CurrentVelocity);

		if (PrintPlayerMovementComponentClientInterpolation)
		{
			B_LOG_DEV("Simulating By DeadReckoning=========================================================================================");
		}
	}
}

void UBPlayerMovementComponent::UpdateAutonomousProxyFromMovementState(float FromServerToClientTime /* RTT / 2 */)
{
	if (PrintPlayerMovementComponentClientInterpolation)
	{
		B_LOG_DEV("UpdateAutonomousProxyFromMovementState=============================================================");
	}

	/*
	 * AutonomousProxy �� ��� ���ĸ��� �Է±��� ��� �˰� �ֱ� ������ ��Ȯ�ϰ� �ùķ��̼� �� �� �ִ�.
	 * RTT / 2 ��ŭ ���̳��� �ùķ��̼��� ó���ϱ� ���� ���� ����� �ƴ�, �ٸ� �۾��� ������ �ʾƵ� �ɰ� ����.
	 * ������ ������� �ʰ�, �������� ���� ����(MovementState)���� RTT / 2 ��ŭ ��ó���� �Է��� �ùķ��̼��Ѵ�.
	 * 
	 * ��Ʈ��ũ ���°� ���� �ʴٸ� ������ Ŭ���̾�Ʈ���� �ùķ��̼��ߴ� �ͺ��� ���� ��ο� ��ġ�ϰ� �� ��������, ���� ������ �������� �ִ��� ���� �� �ִ�.
	 * ��Ʈ��ũ ���°� ���� ������ Ŭ���̾�Ʈ���� ������ �ʰ� ��� ��ó���� �Է��� �ùķ��̼��� ���� ������, �������� �������� �ռ���. ���� ������� �ִ�. */

	// ���������� ó���� �Է��� �����Ѵ�.
	RemoveProcessedMovementObjects(MovementState.LastMovementObjectSharedWorldTime);

	// �������� ���� ���¸� �ٷ� �����ϰ�, ������ ��ó���� �Է��� �ùķ��̼��Ѵ�.
	GetOwner()->SetActorTransform(MovementState.Tranform);
	SetVelocity(MovementState.Velocity);

	// ���������� WorldTime�� �����Ͽ�, �� �ð����� �Է��� �ùķ��̼��Ͽ� �������� �������� ����ȭ�Ѵ�. (���� : ���ø����̼ǵ� ������ WorldTime + RTT / 2)
	const float ExpectedServerSharedWorldTime = MovementState.LastMovementObjectSharedWorldTime + FromServerToClientTime;
	for (const FPlayerMovementObject& MovementObject : PendingMovementObjects)
	{
		if (ExpectedServerSharedWorldTime < MovementObject.SharedWorldTime)
		{
			break;
		}
		SimulateMovementObject(MovementObject);
	}
}

void UBPlayerMovementComponent::RemoveProcessedMovementObjects(float LastMovementObjectSharedWorldTime)
{
	TArray<FPlayerMovementObject> NewPendingMovementObjects;

	const int32 MovementObjectCount = PendingMovementObjects.Num();
	int32 CurrentMovementObjectIndex = 0;

	for (; CurrentMovementObjectIndex < MovementObjectCount; ++CurrentMovementObjectIndex)
	{
		if (PendingMovementObjects[CurrentMovementObjectIndex].SharedWorldTime > LastMovementObjectSharedWorldTime)
		{
			break;
		}
	}

	for (; CurrentMovementObjectIndex < MovementObjectCount; ++CurrentMovementObjectIndex)
	{
		NewPendingMovementObjects.Add(PendingMovementObjects[CurrentMovementObjectIndex]);
	}

	PendingMovementObjects = NewPendingMovementObjects;
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

float UBPlayerMovementComponent::GetDeltaTranslationScalar(float DeltaTime, const FVector& CurrentVelocity) const
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

void UBPlayerMovementComponent::SimulateMovementObject(const FPlayerMovementObject& MovementObject)
{
	const float DeltaTime = MovementObject.DeltaTime;
	const float ForwardMovementFactor = MovementObject.ForwardMovementFactor;
	const float RightMovementFactor = MovementObject.RightMovementFactor;

	if (PrintPlayerMovementComponentVelocity)
	{
		B_LOG_DEV("%.1f, %.1f", ForwardMovementFactor, RightMovementFactor);
	}

	UpdateVelocity(DeltaTime, ForwardMovementFactor, RightMovementFactor);
	UpdateTransform(DeltaTime, ForwardMovementFactor, RightMovementFactor, MovementObject.ControlRotation);
}

void UBPlayerMovementComponent::SetVelocity(const FVector& NewVelocity)
{
	// �߰� ���� �ʿ��ϸ� �߰�
	Velocity = NewVelocity;
}

void UBPlayerMovementComponent::UpdateVelocity(float DeltaTime, float ForwardMovementFactor, float RightMovementFactor)
{
	ApplyInputToVelocity(DeltaTime, ForwardMovementFactor, RightMovementFactor);
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

void UBPlayerMovementComponent::ApplyInputToVelocity(float DeltaTime, float ForwardMovementFactor, float RightMovementFactor)
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

	if (PrintPlayerMovementComponentVelocity)
	{
		B_LOG_DEV("ApplyInputToVelocity=========================================================================================");
		B_LOG_DEV("InputWorldDirection : %.1f, %.1f, %.1f", InputWorldDirection.X, InputWorldDirection.Y, InputWorldDirection.Z);
		B_LOG_DEV("InputDeltaVelocity : %.1f, %.1f, %.1f", InputDeltaVelocity.X, InputDeltaVelocity.Y, InputDeltaVelocity.Z);

		B_LOG_DEV("Final Velocity : %.1f, %.1f, %.1f", Velocity.X, Velocity.Y, Velocity.Z);
	}
}

float UBPlayerMovementComponent::ConvertToControlRotationRange(float TargetAngle) const
{
	// ���е��� �Ҽ��� 2�ڸ����� �����Ѵ�.
	static const int AngleFactor = 100;
	static const int MaxAngle = 360 * AngleFactor;

	int IntAngle = TargetAngle * AngleFactor;
	IntAngle %= MaxAngle;

	if (0.0f > TargetAngle)
	{
		IntAngle += MaxAngle;
	}

	return ((float)IntAngle / AngleFactor);
}

void UBPlayerMovementComponent::UpdateTransform(float DeltaTime, 
	float ForwardMovementFactor, float RightMovementFactor, const FRotator& ControlRotation)
{
	const float DeltaTranslationScalar = GetDeltaTranslationScalar(DeltaTime, Velocity);
	UpdateRotation(DeltaTranslationScalar, ControlRotation);
	UpdateLocation(DeltaTranslationScalar);
}

void UBPlayerMovementComponent::UpdateRotation(float DeltaTranslationScalar, const FRotator& ControlRotation)
{
	Cast<APawn>(GetOwner())->GetControlRotation();
	/**
	 * ActorRotation�� Quaternion���� �Ǿ��ְ�, Rotator�� ��ȯ�� ��, Yaw�� Atan2�Լ��� ����Ͽ� ��ȯ�Ѵ�.
	 * ���� ActionRotation�� [-180, 180]������ ������ ������.
	 * ControlRotation�� [0, 360]�� ������ ������.
	 * CurrentYaw�� ������ �ΰ� [0, 360]�� ���� �������� ����Ѵ�. 
	 * ���������� CurrentYaw�� ���� ActorRotation�� �����Ѵ�. */

	ABPlayer* OwnerPlayer = Cast<ABPlayer>(GetOwner());

	/** �Է� �����Ϳ��� ��� */
	//const FRotator ControlRot = OwnerPlayer->GetControlRotation();
	float TargetYaw = ControlRotation.Yaw;

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
}
// End : Transform, Physics ���� �ڵ� ======================================================================================
