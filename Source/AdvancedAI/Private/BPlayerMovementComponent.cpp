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

	// MaxVelocity 기준으로 계산한다.
	// MovementForceScalar = 100000;	// 100000cN (㎏ × (cm/s^2))cN
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

	// 속도 단위가 cm기준인데, 속도^2하는 식이 나오기 때문에 100으로 나눠준다. 정확하게 확인할 필요가 있다.
	DragCoefficient /= 100.0f;

	//FrictionCoefficient /= 100.0f;

	// cm / s^2 단위로 반환
	DefaultGravityScalar = FMath::Abs(Owner->GetWorld()->GetGravityZ());
	//DefaultMass = GetCharacterMovement()->Mass;

	Velocity = FVector::ZeroVector;

	CurrentYaw = Owner->GetActorRotation().Yaw;
	// ControlRotation과 기준을 맞춘다.
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
	
	// 클라이언트나 서버가 컨트롤하고 있는 폰일 경우
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
		 * 클라이언트에서 컨트롤되고 있는 경우, 현재 입력을 클라이언트에서 시뮬레이션하고, 입력을 서버로 보내주고 입력 리스트를 관리한다.
		 * 이후에 클라이언트의 입력은 서버에서도 시뮬레이션되고, MovementState가 리플리케이션된다.
		 *
		 * 서버에서 컨트롤되고 있는 경우 바로 시뮬레이션을 진행하고, MovementState를 리플리케이션한다.
		 * (앞에서 현재 시스템에서 컨트롤된다는 것을 체크했기 때문에 Role만 ROLE_Authority임을 확인하면 서버에서 컨트롤되고 있는 폰임을 알 수 있다.)
		*/

		// 서버든 클라이언트든 일단 로컬에서 시뮬레이션한다.
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
	else if(CurrentActorRole == ROLE_SimulatedProxy) // 시뮬레이션 중인 폰이라면 서버에서 받은 MovementState를 참고하여 보간을 진행한다.
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
	// [-180, 180 으로 변환]
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


// Begin : Replication 관련 코드 =============================================================================================

bool UBPlayerMovementComponent::GetSharedWorldTime(float& SharedWorldTime) const
{
	const AGameStateBase* CurrentGameState = nullptr != GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (nullptr == CurrentGameState)
	{
		B_ASSERT_DEV(false, "GameState 이나 World 가 존재하지 않습니다.")
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
		B_ASSERT_DEV(false, "클라이언트 키보드 입력 값이 올바르지 않습니다. 강제 로그아웃 처리합니다.")
		return false;
	}

	float ServerSimulatedTime = 0.0f;
	if (false == GetSharedWorldTime(ServerSimulatedTime))
	{
		B_ASSERT_DEV(false, "서버에서 시뮬레이션 시간을 받아올 수 없습니다. 일단 강제 로그아웃 처리합니다.")
		return false;
	}

	const float CurrentClientSimulatedTimeInServer = ClientSimulatedTimeInServer + MovementObject.DeltaTime;
	if (ServerSimulatedTime < CurrentClientSimulatedTimeInServer)
	{
		B_ASSERT_DEV(false, "클라이언트의 시뮬레이션 진행 시간이 비정상입니다. 강제 로그아웃 처리합니다.")
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
	 * TODO : 서버와 클라이언트의 동기화된 시간 변수가 서버에서 얼마나 자주 클라이언트로 리플리케이션 되는 지 확인해야 한다.
	 * 동기화가 잘 되지 않는다면 RTT / 2 를 계산할 때, 다른 방법을 사용해야한다.
	 *
	 * 다른 방법
	 *  : 클라이언트에서 패킷을 보낼 때 타임 스탬프를 찍어 보내고, 서버에서 다시 클라이언트로 보낼 때 해당 타임 스탬프를 그대로 보낸다.
	 *  : 클라이언트에서 해당 타임 스탬프를 기준으로 RTT를 구하고 RTT / 2 를 구한다 */

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
	 * SimulatedProxy 의 경우 서버에서 받은 MovementState가 유지될 것이라는 가정하에 시뮬레이션을 진행한다.
	 * 먼저 RTT / 2 만큼 차이나기 때문에 MovementState를 기반으로 RTT / 2 만큼 시뮬레이션해준다.
	 * SimulatedProxy의 경우 시뮬레이션 예측이 자주 실패하기 때문에 서버에서 받은 상태와 현재 상태가 동일하지 않을 가능성이 높다.
	 * 원하는 상태와 동일하지 않을 때 그 차이가 크다면 그냥 텔리포트한다.
	 * 나머지 경우에는 삼차 스플라인 곡선(삼차 에르미트 스플라인)을 만들어서 다음 상태를 받는 시점에 클라이언트가 예측한 상태로 보간한다.
	 */

	AActor* Owner = GetOwner();

	FVector CurrentVelocity = GetVelocity();
	FVector CurrentLocation = GetOwner()->GetActorLocation();

	// 이전에 보간을 진행하던 목표 상태과 현재 상태에서 위치가 100cm 이상 차이나면, 이전 목표 상태로 강제로 세팅한다.
	if (10000.0f < (CurrentLocation - CubicSpline.TargetLocation).SizeSquared())
	{
		SetTargetMovementBySpline(CubicSpline);
		Owner->SetActorRotation(TargetRotation);
	}

	StartRotation = Owner->GetActorQuat();
	TargetRotation = MovementState.Tranform.GetRotation();
	FVector TargetVelocity = MovementState.Velocity;
	FVector TargetLocation = MovementState.Tranform.GetLocation();

	// RTT / 2 를 사용하지 않고 과거의 주기로 미래를 예측해서 시뮬레이션한다.
	// 다음 상태는 현재 패킷이 도착하기까지 걸린 시간과 비슷하게 걸린다고 가정한다.
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

	// 더 계산해야하면 추가
}

void UBPlayerMovementComponent::InterpolateFromClient(float DeltaTime)
{
	//B_LOG_DEV("InterpolateFromClient => DeltaTime : %.1f", DeltaTime);

	CurrentInterpolationTime += DeltaTime;

	AActor* OwnerActor = GetOwner();
	if (0.0f < InterpolationCompletionTime) // 보간중인 상태
	{
		const float InterpolationRatio = FMath::Clamp(CurrentInterpolationTime / InterpolationCompletionTime, 0.0f, 1.0f);
		if (1.0f == InterpolationRatio)
		{
			InterpolationCompletionTime = 0.0f;
		}

		// 위치, 속도 스플라인 보간
		OwnerActor->SetActorLocation(CubicSpline.InterpolateLocation(InterpolationRatio));
		SetVelocity(CubicSpline.InterpolateVelocity(InterpolationRatio));

		// 회전 구면 보간
		const FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, InterpolationRatio);
		OwnerActor->SetActorRotation(NewRotation);

		if (PrintPlayerMovementComponentClientInterpolation)
		{
			B_LOG_DEV("Interpolating=========================================================================================");
			B_LOG_DEV("InterpolationRatio : %.1f", InterpolationRatio);
		}
	}
	else // 보간완료 상태이기 때문에 현재 속도 기반으로 시뮬레이션한다.
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
	 * AutonomousProxy 의 경우 미쳐리된 입력까지 모두 알고 있기 때문에 정확하게 시뮬레이션 할 수 있다.
	 * RTT / 2 만큼 차이나는 시뮬레이션을 처리하기 위해 보간 방법이 아닌, 다른 작업을 해주지 않아도 될것 같다.
	 * 보간을 사용하지 않고, 서버에서 받은 상태(MovementState)에서 RTT / 2 만큼 미처리된 입력을 시뮬레이션한다.
	 * 
	 * 네트워크 상태가 좋지 않다면 기존에 클라이언트에서 시뮬레이션했던 것보다 이전 경로에 위치하게 될 것이지만, 현재 서버와 움직임을 최대한 맞출 수 있다.
	 * 네트워크 상태가 좋지 않으면 클라이언트에서 끊기지 않게 모든 미처리된 입력을 시뮬레이션할 수도 있지만, 서버보다 움직임이 앞선다. 각각 장단점이 있다. */

	// 서버에서도 처리된 입력은 삭제한다.
	RemoveProcessedMovementObjects(MovementState.LastMovementObjectSharedWorldTime);

	// 서버에서 받은 상태를 바로 적용하고, 나머지 미처리된 입력을 시뮬레이션한다.
	GetOwner()->SetActorTransform(MovementState.Tranform);
	SetVelocity(MovementState.Velocity);

	// 서버에서의 WorldTime을 예측하여, 그 시간까지 입력을 시뮬레이션하여 서버와의 움직임을 동기화한다. (예측 : 리플리케이션된 서버의 WorldTime + RTT / 2)
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
// End : Replication 관련 코드 =============================================================================================


// Begin : Transform, Physics 관련 코드 ======================================================================================
void UBPlayerMovementComponent::RefreshMovementVariable()
{
	CurrentMaxVelocityScalar = MaxVelocityScalar * CurrentMaxVelocityFactor;

	// 최대 속력일 때 저항하는 힘과 같은 힘을 줘야한다.
	MovementForceScalar = GetResistanceScalar(CurrentMaxVelocityScalar);
	AccelerationScalar = MovementForceScalar / DefaultMass;
}

float UBPlayerMovementComponent::GetDeltaTranslationScalar(float DeltaTime, const FVector& CurrentVelocity) const
{
	return CurrentVelocity.Size() * DeltaTime;
}

float UBPlayerMovementComponent::GetResistanceScalar(const float VelocityScalar /* Speed */) const
{
	// 공기 저항 = (속도와 반대 방향) * 속도^2 * 항력계수
	const float AirResistanceScalar = GetAirResistanceScalar(VelocityScalar);

	// 마찰력 = (속도와 반대 방향) * 수직항력 * 마찰계수 // 수직항력의 경우 지면을 수평으로 간주하고 계산한다. (M * G)
	const float FrictionResistanceScalar = GetFrictionResistanceScalar();

	/** 마찰력, 공기저항 계산 */
	const float CurrentResistanceScalar = AirResistanceScalar + FrictionResistanceScalar;
	return CurrentResistanceScalar;
}

float UBPlayerMovementComponent::GetAirResistanceScalar(const float VelocityScalar /* Speed */) const
{
	// 공기 저항 = (속도와 반대 방향) * 속도^2 * 항력계수
	return VelocityScalar * VelocityScalar * DragCoefficient;
}

float UBPlayerMovementComponent::GetFrictionResistanceScalar() const
{
	// 마찰력 = (속도와 반대 방향) * 수직항력 * 마찰계수 // 수직항력의 경우 지면을 수평으로 간주하고 계산한다. (M * G)
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
	// 추가 검증 필요하면 추가
	Velocity = NewVelocity;
}

void UBPlayerMovementComponent::UpdateVelocity(float DeltaTime, float ForwardMovementFactor, float RightMovementFactor)
{
	ApplyInputToVelocity(DeltaTime, ForwardMovementFactor, RightMovementFactor);
	ApplyResistanceToVelocity(DeltaTime);
}

void UBPlayerMovementComponent::ApplyResistanceToVelocity(float DeltaTime)
{
	/** 저항 계산 */
	const float VelocityScalar = Velocity.Size();
	if (VelocityScalar < SMALL_NUMBER)
	{
		return;
	}

	const float ResistanceAccelerationScalar = (GetResistanceScalar(VelocityScalar) / DefaultMass);
	const float DeltaResistanceVelocityScalar = DeltaTime * ResistanceAccelerationScalar;

	if (VelocityScalar <= DeltaResistanceVelocityScalar)
	{ // 멈춘 후에도 힘이 작용하면 안된다.
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
	// 정밀도를 소수점 2자리까지 보장한다.
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
	 * ActorRotation은 Quaternion으로 되어있고, Rotator로 변환할 때, Yaw는 Atan2함수를 사용하여 변환한다.
	 * 따라서 ActionRotation은 [-180, 180]까지의 범위를 가진다.
	 * ControlRotation은 [0, 360]의 범위를 가진다.
	 * CurrentYaw를 변수로 두고 [0, 360]의 같은 범위에서 계산한다. 
	 * 최종적으로 CurrentYaw를 통해 ActorRotation을 설정한다. */

	ABPlayer* OwnerPlayer = Cast<ABPlayer>(GetOwner());

	/** 입력 데이터에서 사용 */
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
}
// End : Transform, Physics 관련 코드 ======================================================================================
