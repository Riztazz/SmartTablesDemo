#include "SmartTableDemoDataTableModel.h"

#include "Logging/StructuredLog.h"
#include "SmartTableLog.h"

bool USmartTableDemoDataTableModel::RecordEdit( int32 NaturalRow, FName ColumnId, const FString & Value )
{
    const FName RowId = GetRowId( NaturalRow );
    if ( RowId.IsNone() )
    {
        UE_LOGFMT( LogSmartTables, Verbose, "Edit on '{Column}' dropped: row {Row} has no row name to file it under.", ColumnId, NaturalRow );
        return false;
    }

    Edits.Add( TPair< FName, FName >( RowId, ColumnId ), Value );

    NotifyRowChanged( NaturalRow );

    return true;
}

void USmartTableDemoDataTableModel::SetCellValue( int32 NaturalRow, FName ColumnId, float Value )
{
    RecordEdit( NaturalRow, ColumnId, FString::SanitizeFloat( Value ) );
}

bool USmartTableDemoDataTableModel::SetCellText_Implementation( int32 NaturalRow, FName ColumnId, const FText & Value )
{
    return RecordEdit( NaturalRow, ColumnId, Value.ToString() );
}

void USmartTableDemoDataTableModel::ForgetEdits()
{
    Edits.Reset();

    NotifyRowsChanged();
}

const FString * USmartTableDemoDataTableModel::FindEdit( int32 NaturalRow, FName ColumnId )
{
    if ( Edits.IsEmpty() )
    {
        return nullptr;
    }

    const FName RowId = GetRowId( NaturalRow );

    return RowId.IsNone() ? nullptr : Edits.Find( TPair< FName, FName >( RowId, ColumnId ) );
}

FText USmartTableDemoDataTableModel::GetCellText_Implementation( int32 NaturalRow, FName ColumnId )
{
    if ( const FString * Edit = FindEdit( NaturalRow, ColumnId ) )
    {
        return FText::FromString( *Edit );
    }

    return Super::GetCellText_Implementation( NaturalRow, ColumnId );
}

FSmartTableSortKey USmartTableDemoDataTableModel::GetCellSortKey_Implementation( int32 NaturalRow, FName ColumnId )
{
    if ( const FString * Edit = FindEdit( NaturalRow, ColumnId ) )
    {
        return Edit->IsNumeric() ? FSmartTableSortKey::MakeNumber( FCString::Atod( **Edit ) ) : FSmartTableSortKey::MakeText( *Edit );
    }

    return Super::GetCellSortKey_Implementation( NaturalRow, ColumnId );
}
