#include "SmartTableDataTablePage.h"

#include "Components/TextBlock.h"
#include "Engine/DataTable.h"
#include "Logging/StructuredLog.h"
#include "SmartTable.h"
#include "SmartTableDemoDataTableModel.h"
#include "SmartTableLog.h"

#define LOCTEXT_NAMESPACE "SmartTablesDemo"

void USmartTableDataTablePage::BeginDataTable( UDataTable * InDataTable )
{
    USmartTable * OwnTable = GetTable();
    checkf( OwnTable, TEXT( "The DataTable page compiled without its BindWidget table" ) );

    if ( !InDataTable )
    {
        UE_LOGFMT( LogSmartTables, Warning, "The DataTable demo has no table to read. Set DemoDataTable on the demo hub Blueprint." );
    }

    Model = NewObject< USmartTableDemoDataTableModel >( this );

    Model->SetColumns( OwnTable->GetColumns() );
    Model->SetDataTable( InDataTable );

    OwnTable->SetModel( Model );

    if ( NoteText )
    {
        NoteText->SetText( LOCTEXT( "DataTableNote", "Edits go where your model puts them" ) );
    }
}

#undef LOCTEXT_NAMESPACE
