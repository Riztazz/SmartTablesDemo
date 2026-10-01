#pragma once

#include "MVVMViewModelBase.h"
#include "SmartTableStationViewModel.generated.h"

UCLASS( BlueprintType, DisplayName = "Smart Table Station Viewmodel" )
class SMARTTABLESDEMO_API USmartTableStationViewModel : public UMVVMViewModelBase
{
    GENERATED_BODY()

public:
    USmartTableStationViewModel();

    UPROPERTY( BlueprintReadOnly, FieldNotify, Category = "Station" )
    FText Station;

    UPROPERTY( BlueprintReadOnly, FieldNotify, Category = "Station" )
    FText Region;

    UPROPERTY( BlueprintReadOnly, FieldNotify, Category = "Station" )
    FText Operator;

    UPROPERTY( BlueprintReadOnly, FieldNotify, Category = "Station" )
    FText Condition;

    UPROPERTY( BlueprintReadOnly, FieldNotify, Category = "Station" )
    FText Remarks;

    UPROPERTY( BlueprintReadOnly, FieldNotify, Category = "Station" )
    double ElevationM = 0.0;

    UPROPERTY( BlueprintReadOnly, FieldNotify, Setter, Category = "Station" )
    double PressureHpa = 0.0;

    UPROPERTY( BlueprintReadOnly, FieldNotify, Setter, Category = "Station" )
    int32 Humidity = 0;

    UPROPERTY( BlueprintReadOnly, FieldNotify, Category = "Station" )
    int32 WindBearing = 0;

    UPROPERTY( BlueprintReadOnly, FieldNotify, Category = "Station" )
    bool bReporting = false;

    UPROPERTY( BlueprintReadOnly, FieldNotify, Category = "Station" )
    FText PressureText;

    UPROPERTY( BlueprintReadOnly, FieldNotify, Category = "Station" )
    float PressureFraction = 0.0f;

    UPROPERTY( BlueprintReadOnly, FieldNotify, Category = "Station" )
    FText HumidityText;

    UPROPERTY( BlueprintReadOnly, FieldNotify, Category = "Station" )
    float HumidityFraction = 0.0f;

    UFUNCTION( BlueprintCallable, Category = "Station" )
    void SetPressureHpa( double NewPressureHpa );

    UFUNCTION( BlueprintCallable, Category = "Station" )
    void SetHumidity( int32 NewHumidity );
};
