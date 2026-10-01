#pragma once

#include "SmartTableDemoRow.h"
#include "SmartTableModel.h"
#include "SmartTableAsyncDemoModel.generated.h"

UCLASS( BlueprintType )
class SMARTTABLESDEMO_API USmartTableAsyncDemoModel : public USmartTableModel
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Demo" )
    void GenerateRows( int32 NumRows );

    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Demo" )
    void UseAsyncSorting( float SimulatedLatencySeconds );

    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Demo" )
    void UseSynchronousSorting();

    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Demo" )
    void UseStatusRowColors( bool bEnabled );

    UFUNCTION( BlueprintPure, Category = "Smart Tables|Demo" )
    int32 GetRowCount() const
    {
        return Rows.Num();
    }

    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Demo" )
    void SetCellValue( int32 NaturalRow, FName ColumnId, float Value );

protected:
    virtual int32 GetNumRows_Implementation() override;
    virtual FText GetCellText_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FSmartTableSortKey GetCellSortKey_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FName GetRowId_Implementation( int32 NaturalRow ) override;
    virtual FLinearColor GetRowColor_Implementation( int32 NaturalRow ) override;

private:
    struct FRow
    {
        double DistanceKm;
        double DeltaV;
        double MassTonnes;
        float Heading;
        float Integrity;
        int32 CrewCapacity;
        bool bVisited;
        ESmartTableDemoStatus Status;
    };

    TArray< FRow > Rows;

    bool bStatusRowColors = false;
};
