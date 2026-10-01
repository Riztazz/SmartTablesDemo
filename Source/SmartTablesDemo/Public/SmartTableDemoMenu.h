#pragma once

#include "SmartTableCell.h"
#include "UObject/Object.h"
#include "SmartTableDemoMenu.generated.h"

UCLASS()
class SMARTTABLESDEMO_API USmartTableDemoMenuItem : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY()
    FString Number;

    UPROPERTY()
    FString Title;

    UPROPERTY()
    FString Blurb;

    int32 DemoIndex = INDEX_NONE;
};

UCLASS()
class SMARTTABLESDEMO_API USmartTableDemoPlayCell : public USmartTableCell
{
    GENERATED_BODY()

protected:
    virtual TSharedRef< SWidget > RebuildWidget() override;
};
