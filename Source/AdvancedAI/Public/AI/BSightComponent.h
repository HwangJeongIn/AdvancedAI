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

	/** ���� �� �ִ� �ִ� �Ÿ� */
	UPROPERTY(Category = "Sight", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0, AllowPrivateAccess = "true"))
	float SightRadius;

	UPROPERTY()
	float SightRadiusSquared;

	/** 
	  * ���� �� �ִ� ���� [0 ~ 180] // ��� -30�̵� 210�̵� ��������� �������� ���� ������ �ξ���.
	  * 0�϶��� �ٷ� ���� �þ߿����� ���̴� �÷��̾ �ν��Ѵ�. 
	  * 180�϶��� SightRadius�ȿ��� ������ �ȴ�. */
	UPROPERTY(Category = "Sight", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0, ClampMax = 180, AllowPrivateAccess = "true"))
	float SightAngle;

	/** �Ź� �ڻ����� ������� �ʱ� ���� SightAngle�� ���� �ڻ��� ���� ���� �����Ѵ�. */
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

	/** �󸶳� ���� �þ߸� ���ؼ� ������ �������� ���� */
	UPROPERTY(Category = "Sight", EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = 0, AllowPrivateAccess = "true"))
	float SeeingInterval;

	/** Ÿ�̸� �����ϴ� �Լ� */
	void SetUpdateSeeingTimer(float InSeeingInterval);

	/** SeeingInterval���� ȣ��Ǵ� �Լ� */
	void OnUpdateSeeing();

	/** SeeingInterval ���� Ÿ�̸Ӱ� ���ư����� ���� */
	FTimerHandle UpdateSeeingTimerHandle;
};
