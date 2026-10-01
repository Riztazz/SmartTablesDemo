#pragma once

#include "GameFramework/Actor.h"
#include "SmartTableDemoStage.generated.h"

class USmartTableDemoHub;

UCLASS()
class SMARTTABLESDEMO_API ASmartTableDemoStage : public AActor
{
    GENERATED_BODY()

public:
    ASmartTableDemoStage();

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Demo", meta = ( ToolTip = "" ) )
    TSubclassOf< USmartTableDemoHub > HubClass;

protected:
    virtual void BeginPlay() override;

private:
    void ShowHub();

    FTimerHandle ShowHandle;

    UPROPERTY( Transient )
    TObjectPtr< USmartTableDemoHub > Hub;
};
