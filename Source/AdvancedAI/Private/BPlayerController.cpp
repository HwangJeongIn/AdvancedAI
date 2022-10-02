// Fill out your copyright notice in the Description page of Project Settings.


#include "BPlayerController.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
//#include "Engine/TimerManager.h"
#include "TimerManager.h"



ABPlayerController::ABPlayerController()
{
    ServerWorldTime = 0.0f;
    RoundTripTime = 0.0f;
    RoundTripTimeHalf = 0.0f;

	RequestDiscardCount = 3;
	RequestNumber = 5;
	CurrentRequestNumber = 0;
}

void ABPlayerController::ReceivedPlayer()
{
    Super::ReceivedPlayer();

    if (IsLocalController())
    {
		//RequestServerWorldTimeHandler();
		ServerRequestServerWorldTime(this, GetWorld()->GetTimeSeconds());
    }
}

void ABPlayerController::ServerRequestServerWorldTime_Implementation(ABPlayerController* Requester, float PrevClientWorldTime)
{
    const float NotifiedServerWorldTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
    ClientNotifyServerWorldTime(PrevClientWorldTime, NotifiedServerWorldTime);
}

bool ABPlayerController::ServerRequestServerWorldTime_Validate(ABPlayerController* Requester, float PrevClientWorldTime)
{
    return true;
}

void ABPlayerController::ClientNotifyServerWorldTime_Implementation(float PrevClientWorldTime, float NotifiedServerWorldTime)
{
	++CurrentRequestNumber;
	if (CurrentRequestNumber > RequestDiscardCount)
	{
		const float CurrentClientWorldTime = GetWorld()->GetTimeSeconds();
		const float NewRoundTripTime = CurrentClientWorldTime - PrevClientWorldTime;
		const float NewRoundTripTimeHalf = (NewRoundTripTime * 0.5f);
		const float NewServerWorldTime = NotifiedServerWorldTime + NewRoundTripTimeHalf;

		const float CurrentCount = CurrentRequestNumber - RequestDiscardCount;
		RoundTripTime = (RoundTripTime * (CurrentCount - 1) + NewRoundTripTime) / CurrentCount;
		RoundTripTimeHalf = (RoundTripTimeHalf * (CurrentCount - 1) + NewRoundTripTimeHalf) / CurrentCount;
		ServerWorldTime = (ServerWorldTime * (CurrentCount - 1) + NewServerWorldTime) / CurrentCount;

		B_LOG_DEV("[%d]NewRoundTripTime : %.6f", CurrentRequestNumber, NewRoundTripTime);
		B_LOG_DEV("[%d]NewServerWorldTime : %.6f", CurrentRequestNumber, NewServerWorldTime);
		B_LOG_DEV("[%d]PrevClientWorldTime : %.6f", CurrentRequestNumber, PrevClientWorldTime);
		B_LOG_DEV("[%d]CurrentClientWorldTime : %.6f", CurrentRequestNumber, CurrentClientWorldTime);
	}

	if (CurrentRequestNumber < RequestNumber)
	{
		//FTimerHandle TempTimerHandle;
		//GetWorld()->GetTimerManager().SetTimer(TempTimerHandle, this,
		//	&ThisClass::RequestServerWorldTimeHandler, 0.5f, false);

		ServerRequestServerWorldTime(this, GetWorld()->GetTimeSeconds());
	}


}

void ABPlayerController::RequestServerWorldTimeHandler()
{
	const float ClientWorldTime = GetWorld()->GetTimeSeconds();
	B_LOG_DEV("[%d]ClientWorldTime : %.6f", CurrentRequestNumber, ClientWorldTime);
	ServerRequestServerWorldTime(this, ClientWorldTime);
}