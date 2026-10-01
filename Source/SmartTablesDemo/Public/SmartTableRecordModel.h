#pragma once

#include "SmartTableModel.h"
#include "SmartTableRecordModel.generated.h"

UCLASS( BlueprintType )
class SMARTTABLESDEMO_API USmartTableRecordModel : public USmartTableModel
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Demo" )
    bool LoadFile( const FString & AbsolutePath );

    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Demo" )
    bool SaveFile();

    const FString & GetLoadedPath() const
    {
        return LoadedPath;
    }

    const TArray< FName > & GetFieldNames() const
    {
        return FieldNames;
    }

protected:
    virtual int32 GetNumRows_Implementation() override;
    virtual FText GetCellText_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FName GetRowId_Implementation( int32 NaturalRow ) override;

    virtual FSmartTableSortKey GetCellSortKey_Implementation( int32 NaturalRow, FName ColumnId ) override;

    virtual ESmartTableCellEditor GetCellEditor_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual bool SetCellText_Implementation( int32 NaturalRow, FName ColumnId, const FText & Value ) override;

private:
    bool LoadCsv( const FString & Contents );
    bool LoadJson( const FString & Contents );

    FString WriteCsv() const;
    FString WriteJson() const;

    FString LoadedPath;

    TArray< TMap< FName, FString > > Records;

    TArray< FName > FieldNames;
};
