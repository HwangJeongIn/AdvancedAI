// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "BAction.generated.h"


class UWorld;
class UBActionComponent;



UENUM(BlueprintType)
enum class EActionType : uint8
{
	PrimaryAttack,
	SecondaryAttack,
	Sprint,
	AIPrimaryAttack
};

USTRUCT()
struct FActionBasicRunningData
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool IsRunning;

	UPROPERTY()
	AActor* InstigatorActor;
};

/**
 * 
 */
UCLASS(Blueprintable)
class ADVANCEDAI_API UBAction : public UObject
{
	GENERATED_BODY()
	
public:
	/**
	  * 기본적으로 UObject는 독립적인 리플리케이션을 지원하지 않는다.
	  * Actor와 연결하여 리플리케이션 해야 한다. (Action을 생성할 때 OwnerActor를 Outer로 설정한다.)
	  * 
	  * 
	  * <Action 객체들을 속성으로 가지고 있는 ActionComponent에서 구현해야 하는 것>
	  * 
	  * ReplicateSubobjects를 재정의하고 내부에서 모든 Action객체에 대한 UActorChannel::ReplicateSubobject를 호출해야 한다.
	  * 그리고 실제로 true를 반환한 객체를 확인하기 위해 함수 반환 값을 | 로 비트연산하여 반환해야 한다.
	  * (액터도 각 컴포넌트(IsReplicated 가 true인 경우)에 대해 ReplicateSubobjects를 호출하고 다시 컴포넌트가 ReplicateSubobjects를 호출하는 방식이다.)
	  * 
	  * <여기서 구현해야 하는 것>
	  * 
	  * IsSupportedForNetworking : 게임에서 도중에 동적으로 생성되는 Action객체 리플리케이션을 위해 재정의 해야한다.
	  * GetFunctionCallspace, CallRemoteFunction : RPC를 사용하려면 재정의가 해야한다.
	  */
	bool IsSupportedForNetworking() const override;

	UWorld* GetWorld() const override;

public:
	virtual void Initialize(UBActionComponent* NewActionComp);	
	
	UFUNCTION(Category = "Action", BlueprintCallable)
	UBActionComponent* GetOwningActionComponent() const;

private:
	UPROPERTY(Replicated)
	UBActionComponent* ActionComp;


public:

	bool IsRunning() const;
	bool CanStart(AActor* InstigatorActor) const;
	bool CanStop(AActor* InstigatorActor) const;

	virtual void Start(AActor* InstigatorActor);
	virtual void Stop(AActor* InstigatorActor);

	bool Compare(const FName& InputName) const;

protected:
	UFUNCTION()
	void OnRep_BasicRunningData();

	UPROPERTY(ReplicatedUsing = "OnRep_BasicRunningData")
	FActionBasicRunningData BasicRunningData;

	UPROPERTY(Category = "Action", EditDefaultsOnly)
	EActionType ActionType;
	
	UPROPERTY(Category = "Action", EditDefaultsOnly)
	FName ActionName;
};
