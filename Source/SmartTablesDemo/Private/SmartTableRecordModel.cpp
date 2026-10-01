#include "SmartTableRecordModel.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Logging/StructuredLog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/Csv/CsvParser.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "SmartTableLog.h"

bool USmartTableRecordModel::LoadFile( const FString & AbsolutePath )
{
    Records.Reset();
    FieldNames.Reset();
    LoadedPath = AbsolutePath;

    FString Contents;
    if ( !FFileHelper::LoadFileToString( Contents, *AbsolutePath ) )
    {
        UE_LOGFMT( LogSmartTables, Warning, "The record model has nothing to show: '{Path}' could not be read.", AbsolutePath );

        NotifyNumRowsChanged();
        return false;
    }

    const bool bLoaded = AbsolutePath.EndsWith( TEXT( ".json" ) ) ? LoadJson( Contents ) : LoadCsv( Contents );

    UE_LOGFMT( LogSmartTables, Verbose, "Record model read {Rows} row(s) of {Fields} field(s) from '{File}'.", Records.Num(), FieldNames.Num(), FPaths::GetCleanFilename( AbsolutePath ) );

    NotifyNumRowsChanged();

    return bLoaded;
}

bool USmartTableRecordModel::LoadCsv( const FString & Contents )
{
    const FCsvParser Parser( Contents );
    const FCsvParser::FRows & Rows = Parser.GetRows();
    if ( Rows.IsEmpty() )
    {
        return false;
    }

    for ( const TCHAR * Cell : Rows[ 0 ] )
    {
        FieldNames.Add( FName( *FString( Cell ).TrimStartAndEnd() ) );
    }

    for ( int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex )
    {
        TMap< FName, FString > Record;

        for ( int32 Field = 0; Field < FieldNames.Num() && Field < Rows[ RowIndex ].Num(); ++Field )
        {
            Record.Add( FieldNames[ Field ], Rows[ RowIndex ][ Field ] );
        }

        Records.Add( MoveTemp( Record ) );
    }

    return true;
}

bool USmartTableRecordModel::LoadJson( const FString & Contents )
{
    const TSharedRef< TJsonReader<> > Reader = TJsonReaderFactory<>::Create( Contents );

    TArray< TSharedPtr< FJsonValue > > Rows;
    if ( !FJsonSerializer::Deserialize( Reader, Rows ) )
    {
        UE_LOGFMT( LogSmartTables, Warning, "That JSON is not an array of rows: {Error}", Reader->GetErrorMessage() );
        return false;
    }

    for ( const TSharedPtr< FJsonValue > & Row : Rows )
    {
        const TSharedPtr< FJsonObject > * Object = nullptr;
        if ( !Row->TryGetObject( Object ) )
        {
            continue;
        }

        TMap< FName, FString > Record;
        for ( const TPair< FString, TSharedPtr< FJsonValue > > & Field : ( *Object )->Values )
        {
            const FName FieldName( *Field.Key );

            if ( Records.IsEmpty() )
            {
                FieldNames.Add( FieldName );
            }

            FString AsText;
            Field.Value->TryGetString( AsText );
            Record.Add( FieldName, AsText );
        }

        Records.Add( MoveTemp( Record ) );
    }

    return true;
}

FString USmartTableRecordModel::WriteCsv() const
{
    TArray< FString > Lines;
    Lines.Reserve( Records.Num() + 1 );

    TArray< FString > Header;
    for ( const FName Field : FieldNames )
    {
        Header.Add( Field.ToString() );
    }

    Lines.Add( FString::Join( Header, TEXT( "," ) ) );

    for ( const TMap< FName, FString > & Record : Records )
    {
        TArray< FString > Cells;
        for ( const FName Field : FieldNames )
        {
            const FString * Value = Record.Find( Field );
            Cells.Add( FString::Printf( TEXT( "\"%s\"" ), Value ? *Value->Replace( TEXT( "\"" ), TEXT( "\"\"" ) ) : TEXT( "" ) ) );
        }

        Lines.Add( FString::Join( Cells, TEXT( "," ) ) );
    }

    return FString::Join( Lines, TEXT( "\n" ) ) + TEXT( "\n" );
}

FString USmartTableRecordModel::WriteJson() const
{
    TArray< TSharedPtr< FJsonValue > > Rows;
    Rows.Reserve( Records.Num() );

    for ( const TMap< FName, FString > & Record : Records )
    {
        const TSharedRef< FJsonObject > Object = MakeShared< FJsonObject >();

        for ( const FName Field : FieldNames )
        {
            const FString * Value = Record.Find( Field );
            Object->SetStringField( Field.ToString(), Value ? *Value : FString() );
        }

        Rows.Add( MakeShared< FJsonValueObject >( Object ) );
    }

    FString Out;
    const TSharedRef< TJsonWriter<> > Writer = TJsonWriterFactory<>::Create( &Out );
    FJsonSerializer::Serialize( Rows, Writer );

    return Out;
}

bool USmartTableRecordModel::SaveFile()
{
    if ( LoadedPath.IsEmpty() )
    {
        UE_LOGFMT( LogSmartTables, Warning, "Nothing was saved: this model has not loaded a file, so it has nowhere to write." );
        return false;
    }

    const FString Contents = LoadedPath.EndsWith( TEXT( ".json" ) ) ? WriteJson() : WriteCsv();

    if ( !FFileHelper::SaveStringToFile( Contents, *LoadedPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM ) )
    {
        UE_LOGFMT( LogSmartTables, Warning, "'{Path}' could not be written. Check it is not read-only.", LoadedPath );
        return false;
    }

    UE_LOGFMT( LogSmartTables, Verbose, "Wrote {Rows} row(s) back to '{File}'.", Records.Num(), FPaths::GetCleanFilename( LoadedPath ) );

    return true;
}

int32 USmartTableRecordModel::GetNumRows_Implementation()
{
    return Records.Num();
}

FText USmartTableRecordModel::GetCellText_Implementation( int32 NaturalRow, FName ColumnId )
{
    if ( !Records.IsValidIndex( NaturalRow ) )
    {
        return FText::GetEmpty();
    }

    const FString * Value = Records[ NaturalRow ].Find( ColumnId );

    return Value ? FText::FromString( *Value ) : FText::GetEmpty();
}

FName USmartTableRecordModel::GetRowId_Implementation( int32 NaturalRow )
{
    return FName( *FString::FromInt( NaturalRow ) );
}

FSmartTableSortKey USmartTableRecordModel::GetCellSortKey_Implementation( int32 NaturalRow, FName ColumnId )
{
    if ( Records.IsValidIndex( NaturalRow ) )
    {
        if ( const FString * Value = Records[ NaturalRow ].Find( ColumnId ) )
        {
            if ( Value->IsNumeric() )
            {
                return FSmartTableSortKey::MakeNumber( FCString::Atod( **Value ) );
            }
        }
    }

    return Super::GetCellSortKey_Implementation( NaturalRow, ColumnId );
}

ESmartTableCellEditor USmartTableRecordModel::GetCellEditor_Implementation( int32 NaturalRow, FName ColumnId )
{
    return Records.IsValidIndex( NaturalRow ) && Records[ NaturalRow ].Contains( ColumnId ) ? ESmartTableCellEditor::Text : ESmartTableCellEditor::None;
}

bool USmartTableRecordModel::SetCellText_Implementation( int32 NaturalRow, FName ColumnId, const FText & Value )
{
    if ( !Records.IsValidIndex( NaturalRow ) || !Records[ NaturalRow ].Contains( ColumnId ) )
    {
        return false;
    }

    Records[ NaturalRow ].Add( ColumnId, Value.ToString() );

    NotifyRowChanged( NaturalRow );

    return true;
}
