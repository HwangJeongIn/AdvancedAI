// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BSightComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSee, AActor*, Target);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class ADVANCEDAI_API UBSightComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UBSightComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void SetIsEnabled(bool NewIsEnabled);
	void SetSightRadius(float NewSightRadius);
	void SetSightAngle(float NewSightAngle);


private:

	UPROPERTY(Category = "Sight", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool IsEnabled;

	/** 들을 수 있는 최대 거리 */
	UPROPERTY(Category = "Sight", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0, AllowPrivateAccess = "true"))
	float SightRadius;

	UPROPERTY()
	float SightRadiusSquared;

	/** 
	  * 들을 수 있는 각도 [0 ~ 180] // 사실 -30이든 210이든 상관없지만 직관성을 위해 제한을 두었다.
	  * 0일때는 바로 직선 시야에서만 보이는 플레이어를 인식한다. 
	  * 180일때는 SightRadius안에만 들어오면 된다. */
	UPROPERTY(Category = "Sight", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0, ClampMax = 180, AllowPrivateAccess = "true"))
	float SightAngle;

	/** 매번 코사인을 계산하지 않기 위해 SightAngle에 따라 코사인 값을 같이 갱신한다. */
	UPROPERTY()
	float SightAngleCosine;


public:
	UFUNCTION(BlueprintCallable)
	void SetSeeingInterval(float NewSeeingInterval);

	UPROPERTY(BlueprintAssignable)
	FOnSee OnSee;

private:
	UFUNCTION()
	void OnSeeTest(AActor* Target);

	void TrySeeing();

	void ValidateTargetAndNotify(AActor* Target);

	UPROPERTY(Category = "Sight", EditDefaultsOnly, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AActor> TargetType;

	/** 얼마나 자주 시야를 통해서 상대방을 인지할지 결정 */
	UPROPERTY(Category = "Sight", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0, AllowPrivateAccess = "true"))
	float SeeingInterval;

	/** 타이머 설정하는 함수 */
	void SetUpdateSeeingTimer(float InSeeingInterval);

	/** SeeingInterval마다 호출되는 함수 */
	void OnUpdateSeeing();

	/** SeeingInterval 마다 타이머가 돌아가도록 설정 */
	FTimerHandle UpdateSeeingTimerHandle;
};
