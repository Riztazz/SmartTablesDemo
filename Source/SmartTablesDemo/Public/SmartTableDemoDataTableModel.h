#pragma once

#include "SmartTableDataTableModel.h"
#include "SmartTableDemoDataTableModel.generated.h"

UCLASS( BlueprintType )
class SMARTTABLESDEMO_API USmartTableDemoDataTableModel : public USmartTableDataTableModel
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Demo" )
    void SetCellValue( int32 NaturalRow, FName ColumnId, float Value );

    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Demo" )
    void ForgetEdits();

protected:
    virtual FText GetCellText_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FSmartTableSortKey GetCellSortKey_Implementation( int32 NaturalRow, FName ColumnId ) override;

    virtual bool SetCellText_Implementation( int32 NaturalRow, FName ColumnId, const FText & Value ) override;

private:
    const FString * FindEdit( int32 NaturalRow, FName ColumnId );

    bool RecordEdit( int32 NaturalRow, FName ColumnId, const FString & Value );

    TMap< TPair< FName, FName >, FString > Edits;
};
