// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BPlayerController.generated.h"


UCLASS()
class ADVANCEDAI_API ABPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
    ABPlayerController();

    float GetServerWorldTime() const { return ServerWorldTime; }
    float GetRoundTripTime() const { return RoundTripTime; }
    float GetRoundTripTimeHalf() const { return RoundTripTimeHalf; }

    virtual void ReceivedPlayer() override;

private:
    UFUNCTION(Client, Reliable)
    void ClientNotifyServerWorldTime(float PrevClientWorldTime, float NotifiedServerWorldTime);

    UFUNCTION(Server, Reliable, WithValidation)
    void ServerRequestServerWorldTime(ABPlayerController* Requester, float PrevClientWorldTime);

	void RequestServerWorldTimeHandler();

    UPROPERTY(VisibleAnywhere)
    float ServerWorldTime;

    UPROPERTY(VisibleAnywhere)
    float RoundTripTime;

    UPROPERTY(VisibleAnywhere)
    float RoundTripTimeHalf;

    UPROPERTY(EditDefaultsOnly)
    int RequestDiscardCount;

    UPROPERTY(EditDefaultsOnly)
    int RequestNumber;

    UPROPERTY(VisibleAnywhere)
	int CurrentRequestNumber;
};
