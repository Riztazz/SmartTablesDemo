#include "SmartTableDemoStage.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Logging/StructuredLog.h"
#include "Misc/AssertionMacros.h"
#include "SmartTableDemoHub.h"
#include "SmartTableLog.h"
#include "TimerManager.h"

ASmartTableDemoStage::ASmartTableDemoStage()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ASmartTableDemoStage::BeginPlay()
{
    Super::BeginPlay();

    GetWorldTimerManager().SetTimer( ShowHandle, this, &ASmartTableDemoStage::ShowHub, 0.5f, false );
}

void ASmartTableDemoStage::ShowHub()
{
    if ( !HubClass )
    {
        UE_LOGFMT( LogSmartTables, Warning, "{Actor} has no HubClass, so there is nothing to show. Set it on the placed actor - a Blueprint deriving from SmartTableDemoHub.", GetName() );

        return;
    }

    checkf( GetWorld(), TEXT( "%s is running BeginPlay with no world" ), *GetName() );

    APlayerController * Controller = GetWorld()->GetFirstPlayerController();
    if ( !Controller )
    {
        UE_LOGFMT( LogSmartTables, Warning, "{Actor} found no player controller to own the demo widget, so nothing is shown. Give the level a PlayerController - the default game mode's will do.", GetName() );

        return;
    }

    Hub = CreateWidget< USmartTableDemoHub >( Controller, HubClass );
    if ( !Hub )
    {
        UE_LOGFMT( LogSmartTables, Warning, "{Actor} could not create a widget of class {Class}, so nothing is shown. Point HubClass at a concrete (non-Abstract) Blueprint deriving from SmartTableDemoHub.", GetName(), HubClass->GetName() );

        return;
    }

    UE_LOGFMT( LogSmartTables, Verbose, "{Actor} put the demo hub on screen.", GetName() );

    Hub->AddToViewport();

    Controller->bShowMouseCursor = true;
    Controller->SetInputMode( FInputModeGameAndUI().SetLockMouseToViewportBehavior( EMouseLockMode::DoNotLock ) );
}
