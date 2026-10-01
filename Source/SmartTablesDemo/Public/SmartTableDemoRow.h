#pragma once

#include "Engine/DataTable.h"
#include "SmartTableDemoRow.generated.h"

UENUM( BlueprintType )
enum class ESmartTableDemoStatus : uint8
{
    Unsurveyed,
    Surveyed,
    Claimed,
    Derelict,
    Restricted
};

USTRUCT( BlueprintType )
struct SMARTTABLESDEMO_API FSmartTableDemoRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Identity", meta = ( ToolTip = "" ) )
    FString Callsign;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Identity", meta = ( ToolTip = "" ) )
    FName Designation;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Identity", meta = ( ToolTip = "" ) )
    FText DisplayName;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Identity", meta = ( ToolTip = "" ) )
    FString Class;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Identity", meta = ( ToolTip = "" ) )
    FString Owner;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Identity", meta = ( ToolTip = "" ) )
    FString Registry;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Identity", meta = ( ToolTip = "" ) )
    FString Notes;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Orbit", meta = ( ToolTip = "" ) )
    double DistanceKm = 0.0;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Orbit", meta = ( ToolTip = "" ) )
    double DeltaV = 0.0;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Orbit", meta = ( ToolTip = "" ) )
    double MassTonnes = 0.0;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Orbit", meta = ( ToolTip = "" ) )
    float Heading = 0.0f;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Orbit", meta = ( ToolTip = "" ) )
    float Inclination = 0.0f;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Orbit", meta = ( ToolTip = "" ) )
    float Eccentricity = 0.0f;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Orbit", meta = ( ToolTip = "" ) )
    float PeriodMinutes = 0.0f;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Condition", meta = ( ToolTip = "" ) )
    float Integrity = 0.0f;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Condition", meta = ( ToolTip = "" ) )
    int32 CrewCapacity = 0;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Condition", meta = ( ToolTip = "" ) )
    int32 DockingPorts = 0;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Condition", meta = ( ToolTip = "" ) )
    int32 LastContactDays = 0;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Condition", meta = ( ToolTip = "" ) )
    bool bVisited = false;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Condition", meta = ( ToolTip = "" ) )
    ESmartTableDemoStatus Status = ESmartTableDemoStatus::Unsurveyed;
};
