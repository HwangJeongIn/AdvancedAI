// Fill out your copyright notice in the Description page of Project Settings.


#include "BPlayerMovementComponent.h"
#include "Engine/World.h"
#include "BPlayer.h"
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


UBPlayerMovementComponent::UBPlayerMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	DefaultMass = 100;					// 100kg
	MaxVelocityScalar = 1000;			// 1000 cm/s = 10 m/s
	CurrentMaxVelocityFactor = 1.0f;
	MaxVelocityScalar = MaxVelocityScalar * CurrentMaxVelocityFactor;

	// MaxVelocity 기준으로 계산한다.
	// MovementForceScalar = 100000;	// 100000cN (㎏ × (cm/s^2))cN
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
						, RightMovementFactor);
		


		/**
		 * 클라이언트에서 컨트롤되고 있는 경우, 현재 입력을 클라이언트에서 시뮬레이션하고, 입력을 서버로 보내주고 입력 리스트를 관리한다.
		 * 이후에 클라이언트의 입력은 서버에서도 시뮬레이션되고, MovementState가 리플리케이션된다.
		 *
		 * 서버에서 컨트롤되고 있는 경우 바로 시뮬레이션을 진행하고, MovementState를 리플리케이션한다.
		 * (앞에서 현재 시스템에서 컨트롤된다는 것을 체크했기 때문에 Role만 ROLE_Authority임을 확인하면 서버에서 컨트롤되고 있는 폰임을 알 수 있다.)
		*/

		switch (CurrentActorRole)
		{
		case ROLE_AutonomousProxy:
		{
			PendingMovementObjects.Add(LastMovementObject);
			ServerMove(LastMovementObject);

			// 클라이언트는 클라이언트대로 시뮬레이션 해야한다.
			SimulateMovementObject(LastMovementObject);
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

/*
bool UBPlayerMovementComponent::CreateMovementObject(float DeltaTime, FPlayerMovementObject& NewMovementObject) const
{
	const AGameStateBase* CurrentGameState = nullptr != GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (nullptr == CurrentGameState)
	{
		return false;
	}

	NewMovementObject.DeltaTime = DeltaTime;
	NewMovementObject.SharedTime = CurrentGameState->GetServerWorldTimeSeconds();

	NewMovementObject.SteeringThrow = SteeringThrow;
	NewMovementObject.Throttle = Throttle;
	return true;
}
*/

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

void UBPlayerMovementComponent::ServerMove_Implementation(const FPlayerMovementObject& MovementObject)
{
	ClientSimulatedTimeInServer += MovementObject.DeltaTime;

	SimulateMovementObject(MovementObject);
	UpdateMovementState(MovementObject);
}

bool UBPlayerMovementComponent::ServerMove_Validate(const FPlayerMovementObject& MovementObject)
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
	MovementState.LastMovementObjectSharedWorldTime = MovementObject.SharedWorldTime;
	MovementState.Tranform = GetOwner()->GetActorTransform();
	MovementState.Velocity = GetVelocity();
	/* const bool Result = */GetSharedWorldTime(MovementState.DepartureTime);
}


void UBPlayerMovementComponent::OnRep_MovementState()
{
	/**
	 * TODO : 서버와 클라이언트의 동기화된 시간 변수가 서버에서 얼마나 자주 클라이언트로 리플리케이션 되는 지 확인해야 한다.
	 * 동기화가 잘 되지 않는다면 RTT / 2 를 계산할 때, 다른 방법을 사용해야한다.
	 *
	 * 다른 방법
	 *  : 클라이언트에서 패킷을 보낼 때 타임 스탬프를 찍어 보내고, 서버에서 다시 클라이언트로 보낼 때 해당 타임 스탬프를 그대로 보낸다.
	 *  : 클라이언트에서 해당 타임 스탬프를 기준으로 RTT를 구하고 RTT / 2 를 구한다 */

	 /** RTT / 2 */
	float FromServerToClientTime = 0.0f;
	float ClientSimulatedTime = 0.0f;
	if (GetSharedWorldTime(ClientSimulatedTime))
	{
		FromServerToClientTime = ClientSimulatedTime - MovementState.DepartureTime;
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

	/*
	 * AutonomousProxy 의 경우 미쳐리된 입력까지 모두 알고 있기 때문에 정확하게 시뮬레이션 할 수 있다.
	 * RTT / 2 만큼 차이나는 시뮬레이션을 처리하기 위해 보간 방법이 아닌, 다른 작업을 해주지 않아도 될것 같다.
	 * 보간을 사용하지 않고, 서버에서 받은 상태(MovementState)에서 RTT / 2 만큼 미처리된 입력을 시뮬레이션한다.

	 */

	//===================================
	/*
	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0;

	if (MeshOffsetRoot != nullptr)
	{
		ClientStartTransform.SetLocation(MeshOffsetRoot->GetComponentLocation());
		ClientStartTransform.SetRotation(MeshOffsetRoot->GetComponentQuat());
	}
	ClientStartVelocity = MovementComponent->GetVelocity();

	GetOwner()->SetActorTransform(ServerState.Tranform);*/
}

void UBPlayerMovementComponent::CalculateMovementByDeadReckoning(float DeltaTime)
{
	TargetLocation += TargetVelocity * DeltaTime;
}

void UBPlayerMovementComponent::InterpolateFromClient(float DeltaTime)
{
	/** Location 보간 */

	/** Velocity 보간 */
}

void UBPlayerMovementComponent::UpdateAutonomousProxyFromMovementState(float FromServerToClientTime /* RTT / 2 */)
{
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

float UBPlayerMovementComponent::GetDeltaTranslationScalar(const FVector& CurrentVelocity, float DeltaTime) const
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

	UpdateVelocity(ForwardMovementFactor, RightMovementFactor, DeltaTime);
	UpdateTransform(ForwardMovementFactor, RightMovementFactor, DeltaTime);
}

void UBPlayerMovementComponent::SetVelocity(const FVector& NewVelocity)
{
	// 추가 검증 필요하면 추가
	Velocity = NewVelocity;
}

void UBPlayerMovementComponent::UpdateVelocity(float ForwardMovementFactor, float RightMovementFactor, float DeltaTime)
{
	ApplyInputToVelocity(ForwardMovementFactor, RightMovementFactor, DeltaTime);
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
	// 정밀도를 소수점 2자리까지 보장한다.
	static const int AngleFactor = 100;
	static const int MaxAngle = 360 * AngleFactor;

	int IntAngle = angle * AngleFactor;
	IntAngle %= MaxAngle;

	if (0.0f > angle)
	{
		IntAngle += MaxAngle;
	}

	return ((float)IntAngle / AngleFactor);
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
	 * 최종적으로 CurrentYaw를 통해 ActorRotation을 설정한다. */

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

// End : Transform, Physics 관련 코드 ======================================================================================
