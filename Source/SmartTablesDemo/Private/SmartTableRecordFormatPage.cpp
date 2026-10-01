#include "SmartTableRecordFormatPage.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Logging/StructuredLog.h"
#include "Misc/Paths.h"
#include "SmartTable.h"
#include "SmartTableLog.h"
#include "SmartTableRecordModel.h"

#define LOCTEXT_NAMESPACE "SmartTablesDemo"

void USmartTableRecordFormatPage::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if ( SaveButton )
    {
        SaveButton->OnClicked.AddDynamic( this, &USmartTableRecordFormatPage::HandleSaveClicked );
    }
}

void USmartTableRecordFormatPage::BeginRecordFormat()
{
    USmartTable * OwnTable = GetTable();
    checkf( OwnTable, TEXT( "A record format page compiled without its BindWidget table" ) );

    Model = NewObject< USmartTableRecordModel >( this );
    Model->LoadFile( FPaths::ProjectContentDir() / ContentRelativePath );

    OwnTable->SetModel( Model );

    if ( StatusText )
    {
        StatusText->SetText( FText::Format( LOCTEXT( "RecordLoaded", "{0}, {1} rows" ), FText::FromString( FPaths::GetCleanFilename( ContentRelativePath ) ), FText::AsNumber( Model->GetNumRows() ) ) );
    }
}

void USmartTableRecordFormatPage::HandleSaveClicked()
{
    SaveNow();
}

bool USmartTableRecordFormatPage::SaveNow()
{
    checkf( Model, TEXT( "A record format page offered Save before it loaded anything" ) );

    const bool bSaved = Model->SaveFile();

    if ( StatusText )
    {
        StatusText->SetText( bSaved ? FText::Format( LOCTEXT( "RecordSaved", "Saved {0} rows to {1}" ), FText::AsNumber( Model->GetNumRows() ), FText::FromString( FPaths::GetCleanFilename( Model->GetLoadedPath() ) ) ) : FText::Format( LOCTEXT( "RecordNotSaved", "Could not write {0}" ), FText::FromString( FPaths::GetCleanFilename( Model->GetLoadedPath() ) ) ) );
    }

    return bSaved;
}

#undef LOCTEXT_NAMESPACE
