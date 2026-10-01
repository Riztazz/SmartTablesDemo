#include "SmartTableDemoPage.h"
#include "SmartTableDemoConstants.h"

#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/InputSettings.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Logging/StructuredLog.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "SmartTable.h"
#include "SmartTableAsyncDemoModel.h"
#include "SmartTableDemoDataTableModel.h"
#include "SmartTableLog.h"
#include "SmartTableModel.h"
#include "SmartTableRecordModel.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

namespace
{
    bool SetFieldFromText( UObject & Item, FName Field, const FString & Value )
    {
        FProperty * Property = FindFProperty< FProperty >( Item.GetClass(), Field );
        if ( !Property )
        {
            return false;
        }

        Property->ImportText_Direct( *Value, Property->ContainerPtrToValuePtr< void >( &Item ), &Item, PPF_None );

        return true;
    }

    const TArray< FName > & StationFields()
    {
        static const TArray< FName > Fields = { TEXT( "Station" ), TEXT( "Region" ), TEXT( "Operator" ), TEXT( "ElevationM" ), TEXT( "PressureHpa" ), TEXT( "bReporting" ), TEXT( "Condition" ), TEXT( "Humidity" ), TEXT( "WindBearing" ), TEXT( "Remarks" ) };

        return Fields;
    }
}

USmartTableDemoPage::USmartTableDemoPage( const FObjectInitializer & ObjectInitializer )
    : Super( ObjectInitializer )
{
    auto Find = []( const TCHAR * Path ) -> UInputAction *
    {
        const ConstructorHelpers::FObjectFinder< UInputAction > Asset( Path );

        return Asset.Succeeded() ? Asset.Object : nullptr;
    };

    const ConstructorHelpers::FObjectFinder< UInputMappingContext > ContextAsset( *SmartTableDemo::SharedAsset( TEXT( "IMC_SmartTableDemo" ) ) );
    InputContext = ContextAsset.Succeeded() ? ContextAsset.Object : nullptr;

    FocusAction      = Find( *SmartTableDemo::SharedAsset( TEXT( "IA_Table_Focus" ) ) );
    UpAction         = Find( *SmartTableDemo::SharedAsset( TEXT( "IA_Table_Up" ) ) );
    DownAction       = Find( *SmartTableDemo::SharedAsset( TEXT( "IA_Table_Down" ) ) );
    ActivateAction   = Find( *SmartTableDemo::SharedAsset( TEXT( "IA_Table_Activate" ) ) );
    MenuAction       = Find( *SmartTableDemo::SharedAsset( TEXT( "IA_Table_Menu" ) ) );
    SortAction       = Find( *SmartTableDemo::SharedAsset( TEXT( "IA_Table_Sort" ) ) );
    WiderAction      = Find( *SmartTableDemo::SharedAsset( TEXT( "IA_Table_Wider" ) ) );
    NarrowerAction   = Find( *SmartTableDemo::SharedAsset( TEXT( "IA_Table_Narrower" ) ) );
    NextColumnAction = Find( *SmartTableDemo::SharedAsset( TEXT( "IA_Table_NextColumn" ) ) );
    ResizeHoldAction = Find( *SmartTableDemo::SharedAsset( TEXT( "IA_Table_ResizeHold" ) ) );

    MoveRowUpAction   = Find( *SmartTableDemo::SharedAsset( TEXT( "IA_Table_MoveRowUp" ) ) );
    MoveRowDownAction = Find( *SmartTableDemo::SharedAsset( TEXT( "IA_Table_MoveRowDown" ) ) );
}

void USmartTableDemoPage::NativeConstruct()
{
    Super::NativeConstruct();

    if ( Table )
    {
        FScriptDelegate CellChanged;
        CellChanged.BindUFunction( this, GET_FUNCTION_NAME_CHECKED( USmartTableDemoPage, HandleCellValueChanged ) );
        Table->OnCellValueChanged.Add( CellChanged );
    }

    APlayerController * Player = GetOwningPlayer();
    if ( !Player )
    {
        return;
    }

    if ( !InputContext )
    {
        return;
    }

    if ( UEnhancedInputLocalPlayerSubsystem * Input = ULocalPlayer::GetSubsystem< UEnhancedInputLocalPlayerSubsystem >( Player->GetLocalPlayer() ) )
    {
        Input->AddMappingContext( InputContext, 0 );
    }

    if ( !InputComponent )
    {
        InputComponent = NewObject< UInputComponent >( this, UInputSettings::GetDefaultInputComponentClass(), NAME_None, RF_Transient );
    }

    UEnhancedInputComponent * Component = Cast< UEnhancedInputComponent >( InputComponent );
    if ( !Component )
    {
        UE_LOGFMT( LogSmartTables, Warning, "Demo page '{Page}' has no Enhanced Input component, so its key bindings do nothing. The project's DefaultInputComponentClass has to be EnhancedInputComponent.", GetName() );
        return;
    }

    Component->BindAction( FocusAction, ETriggerEvent::Started, this, &USmartTableDemoPage::HandleFocus );
    Component->BindAction( UpAction, ETriggerEvent::Started, this, &USmartTableDemoPage::HandleUp );
    Component->BindAction( DownAction, ETriggerEvent::Started, this, &USmartTableDemoPage::HandleDown );
    Component->BindAction( ActivateAction, ETriggerEvent::Started, this, &USmartTableDemoPage::HandleActivate );
    Component->BindAction( MenuAction, ETriggerEvent::Started, this, &USmartTableDemoPage::HandleMenu );
    Component->BindAction( SortAction, ETriggerEvent::Started, this, &USmartTableDemoPage::HandleSort );
    Component->BindAction( WiderAction, ETriggerEvent::Started, this, &USmartTableDemoPage::HandleWider );
    Component->BindAction( NarrowerAction, ETriggerEvent::Started, this, &USmartTableDemoPage::HandleNarrower );
    Component->BindAction( NextColumnAction, ETriggerEvent::Started, this, &USmartTableDemoPage::HandleNextColumn );

    if ( MoveRowUpAction && MoveRowDownAction )
    {
        Component->BindAction( MoveRowUpAction, ETriggerEvent::Started, this, &USmartTableDemoPage::HandleMoveRowUp );
        Component->BindAction( MoveRowDownAction, ETriggerEvent::Started, this, &USmartTableDemoPage::HandleMoveRowDown );
    }

    if ( bDemonstrateHeldResize && ResizeHoldAction )
    {
        Component->BindAction( ResizeHoldAction, ETriggerEvent::Started, this, &USmartTableDemoPage::HandleHoldBegun );
        Component->BindAction( ResizeHoldAction, ETriggerEvent::Triggered, this, &USmartTableDemoPage::HandleHoldStep );
        Component->BindAction( ResizeHoldAction, ETriggerEvent::Completed, this, &USmartTableDemoPage::HandleHoldEnded );
        Component->BindAction( ResizeHoldAction, ETriggerEvent::Canceled, this, &USmartTableDemoPage::HandleHoldEnded );
    }
    else if ( bDemonstrateHeldResize )
    {
        UE_LOGFMT( LogSmartTables, Warning, "Demo page '{Page}' asks for the held resize but has no ResizeHoldAction, so the key does nothing. Set it to /Game/Demo/Shared/IA_Table_ResizeHold.", GetName() );
    }

    UE_LOGFMT( LogSmartTables, Verbose, "Demo page '{Page}' input bound. Held resize: {State}.", GetName(), !bDemonstrateHeldResize ? TEXT( "off" ) : ( ResizeHoldAction ? TEXT( "bound" ) : TEXT( "asked for, but the action asset is missing" ) ) );

    RegisterInputComponent();
}

void USmartTableDemoPage::HandleCellValueChanged( UObject * Item, int32 NaturalRow, FName ColumnId, float Value )
{
    if ( !Table )
    {
        return;
    }

    if ( Item && SetFieldFromText( *Item, ColumnId, LexToString( Value ) ) )
    {
        Table->NotifyItemChanged( Item );

        return;
    }

    if ( USmartTableDemoDataTableModel * Rows = Cast< USmartTableDemoDataTableModel >( Table->GetModel() ) )
    {
        Rows->SetCellValue( NaturalRow, ColumnId, Value );

        return;
    }

    if ( USmartTableRecordModel * Records = Cast< USmartTableRecordModel >( Table->GetModel() ) )
    {
        Records->SetCellText( NaturalRow, ColumnId, FText::FromString( FString::SanitizeFloat( Value ) ) );

        return;
    }

    if ( USmartTableAsyncDemoModel * Async = Cast< USmartTableAsyncDemoModel >( Table->GetModel() ) )
    {
        Async->SetCellValue( NaturalRow, ColumnId, Value );

        return;
    }

    UE_LOGFMT( LogSmartTables, Verbose, "An edit on row {Row} of '{Column}' had nowhere to go: '{Page}' is showing a model that holds no data of its own.", NaturalRow, ColumnId, GetName() );
}

void USmartTableDemoPage::NativeDestruct()
{
    if ( Table )
    {
        Table->OnCellValueChanged.RemoveAll( this );
        Table->EndColumnResize();
    }

    UnregisterInputComponent();

    if ( APlayerController * Player = GetOwningPlayer() )
    {
        if ( UEnhancedInputLocalPlayerSubsystem * Input = ULocalPlayer::GetSubsystem< UEnhancedInputLocalPlayerSubsystem >( Player->GetLocalPlayer() ) )
        {
            if ( InputContext )
            {
                Input->RemoveMappingContext( InputContext );
            }
        }
    }

    Super::NativeDestruct();
}

void USmartTableDemoPage::HandleNextColumn()
{
    Table->MoveColumnSelection( 1 );
}

void USmartTableDemoPage::HandleFocus()
{
    Table->FocusTable();
}

void USmartTableDemoPage::HandleUp()
{
    Table->MoveSelection( -1 );
}

void USmartTableDemoPage::HandleDown()
{
    Table->MoveSelection( 1 );
}

void USmartTableDemoPage::HandleActivate()
{
    Table->ActivateSelectedRow();
}

void USmartTableDemoPage::HandleMenu()
{
    Table->OpenHeaderMenu( NAME_None );
}

void USmartTableDemoPage::HandleSort()
{
    Table->CycleColumnSort( NAME_None );
}

void USmartTableDemoPage::StepResize( float DeltaPixels )
{
    Table->BeginColumnResize( NAME_None );
    Table->UpdateColumnResize( DeltaPixels );
    Table->EndColumnResize();
}

void USmartTableDemoPage::HandleHoldBegun()
{
    Table->BeginColumnResize( NAME_None );
}

void USmartTableDemoPage::HandleHoldStep()
{
    const UWorld * World = GetWorld();

    Table->UpdateColumnResize( ResizeHoldPixelsPerSecond * ( World ? World->GetDeltaSeconds() : 0.0f ) );
}

void USmartTableDemoPage::HandleHoldEnded()
{
    Table->EndColumnResize();
}

void USmartTableDemoPage::HandleWider()
{
    StepResize( ResizeStepPixels );
}

void USmartTableDemoPage::HandleNarrower()
{
    StepResize( -ResizeStepPixels );
}

USmartTableKeyboardDemoPage::USmartTableKeyboardDemoPage( const FObjectInitializer & ObjectInitializer )
    : Super( ObjectInitializer )
{
    InputContext = nullptr;

    SetIsFocusable( true );
}

void USmartTableKeyboardDemoPage::NativeConstruct()
{
    Super::NativeConstruct();

    if ( APlayerController * Player = GetOwningPlayer() )
    {
        SetUserFocus( Player );
    }
}

void USmartTableDemoPage::HandleMoveRowUp()
{
    if ( Table )
    {
        Table->MoveSelectedRows( -1 );
    }
}

void USmartTableDemoPage::HandleMoveRowDown()
{
    if ( Table )
    {
        Table->MoveSelectedRows( 1 );
    }
}

FReply USmartTableKeyboardDemoPage::NativeOnKeyDown( const FGeometry & Geometry, const FKeyEvent & KeyEvent )
{
    const FKey Key = KeyEvent.GetKey();

    if ( Key == EKeys::F )
    {
        HandleFocus();
    }
    else if ( Key == EKeys::R )
    {
        HandleSort();
    }
    else if ( Key == EKeys::M )
    {
        HandleMenu();
    }
    else if ( Key == EKeys::N )
    {
        HandleNextColumn();
    }
    else if ( Key == EKeys::Period )
    {
        HandleWider();
    }
    else if ( Key == EKeys::Comma )
    {
        HandleNarrower();
    }
    else if ( Key == EKeys::LeftBracket )
    {
        HandleMoveRowUp();
    }
    else if ( Key == EKeys::RightBracket )
    {
        HandleMoveRowDown();
    }
    else
    {
        return Super::NativeOnKeyDown( Geometry, KeyEvent );
    }

    return FReply::Handled();
}

void USmartTableDemoPage::DumpRowsToCsv() const
{
    const TConstArrayView< TObjectPtr< UObject > > Rows = Table ? Table->GetItems() : TConstArrayView< TObjectPtr< UObject > >();
    if ( Rows.IsEmpty() )
    {
        return;
    }

    TArray< FString > Lines;
    Lines.Reserve( Rows.Num() + 1 );

    FString Header;
    for ( const FName & Field : StationFields() )
    {
        Header += Header.IsEmpty() ? Field.ToString() : TEXT( "," ) + Field.ToString();
    }
    Lines.Add( Header );

    for ( const TObjectPtr< UObject > & Row : Rows )
    {
        FString Line;
        for ( const FName & Field : StationFields() )
        {
            FString Value;
            if ( const FProperty * Property = FindFProperty< FProperty >( Row->GetClass(), Field ) )
            {
                Property->ExportTextItem_Direct( Value, Property->ContainerPtrToValuePtr< void >( Row ), nullptr, nullptr, PPF_None );
            }

            Line += FString::Printf( TEXT( "%s\"%s\"" ), Line.IsEmpty() ? TEXT( "" ) : TEXT( "," ), *Value.ReplaceCharWithEscapedChar() );
        }

        Lines.Add( Line );
    }

    const FString Path = FPaths::ProjectSavedDir() / TEXT( "SmartTables" ) / TEXT( "Stations.csv" );
    if ( FFileHelper::SaveStringArrayToFile( Lines, *Path, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM ) )
    {
        UE_LOGFMT( LogSmartTables, Verbose, "Dumped {Rows} row(s) to '{Path}'.", Rows.Num(), Path );
    }
    else
    {
        UE_LOGFMT( LogSmartTables, Warning, "Could not write the row dump to '{Path}'. Check the folder is writable.", Path );
    }
}

namespace
{
    const TCHAR * StationNames[]   = { TEXT( "Cairn Head" ), TEXT( "Blackrigg" ), TEXT( "Sennen" ), TEXT( "Ardvar" ), TEXT( "Peel Fell" ), TEXT( "Ganton" ) };
    const TCHAR * StationRegions[] = { TEXT( "Highlands" ), TEXT( "Fens" ), TEXT( "West Coast" ), TEXT( "Pennines" ) };
    const TCHAR * StationCrews[]   = { TEXT( "Met Office" ), TEXT( "Coastal Trust" ), TEXT( "Volunteer Net" ) };
    const TCHAR * StationSkies[]   = { TEXT( "Clear" ), TEXT( "Overcast" ), TEXT( "Rain" ), TEXT( "Fog" ), TEXT( "Storm" ) };
    const TCHAR * StationRemarks[] = {
        TEXT( "Reports on the hour only." ),
        TEXT( "Anemometer ices over below freezing." ),
        TEXT( "Solar panel shaded by the new mast after midday." ),
    };

    void Write( UObject & Item, const TCHAR * Field, const FString & Value )
    {
        if ( FProperty * Property = FindFProperty< FProperty >( Item.GetClass(), Field ) )
        {

            Property->ImportText_InContainer( *Value, &Item, &Item, PPF_None );
        }
    }

    FString Read( const UObject & Item, const TCHAR * Field )
    {
        FString Value;
        if ( const FProperty * Property = FindFProperty< FProperty >( Item.GetClass(), Field ) )
        {
            Property->ExportTextItem_Direct( Value, Property->ContainerPtrToValuePtr< void >( &Item ), nullptr, nullptr, PPF_None );
        }

        return Value;
    }
}

UObject * USmartTableDemoPage::MakeSampleRow( int32 Index )
{
    UClass * RowClass = SampleRowClass.LoadSynchronous();
    if ( !RowClass )
    {
        UE_LOGFMT( LogSmartTables, Warning, "Page {Page} builds no sample rows. Set Sample Row Class in its Class Defaults.", GetClass()->GetName() );
        return nullptr;
    }

    UObject * Item = NewObject< UObject >( this, RowClass );

    Write( *Item, TEXT( "Station" ), StationNames[ Index % UE_ARRAY_COUNT( StationNames ) ] );
    Write( *Item, TEXT( "Region" ), StationRegions[ Index % UE_ARRAY_COUNT( StationRegions ) ] );
    Write( *Item, TEXT( "Operator" ), StationCrews[ Index % UE_ARRAY_COUNT( StationCrews ) ] );
    Write( *Item, TEXT( "Condition" ), StationSkies[ Index % UE_ARRAY_COUNT( StationSkies ) ] );
    Write( *Item, TEXT( "Remarks" ), StationRemarks[ Index % UE_ARRAY_COUNT( StationRemarks ) ] );
    Write( *Item, TEXT( "ElevationM" ), LexToString( Index * 47.0 ) );
    Write( *Item, TEXT( "PressureHpa" ), LexToString( 958.0 + ( Index % 92 ) ) );
    Write( *Item, TEXT( "Humidity" ), LexToString( ( Index * 17 ) % 101 ) );
    Write( *Item, TEXT( "WindBearing" ), LexToString( ( Index * 23 ) % 360 ) );
    Write( *Item, TEXT( "bReporting" ), ( Index % 5 ) == 0 ? TEXT( "False" ) : TEXT( "True" ) );

    return Item;
}

TArray< UObject * > USmartTableDemoPage::MakeSampleRows()
{
    TArray< UObject * > Rows;
    Rows.Reserve( SampleRowCount );

    for ( int32 Index = 0; Index < SampleRowCount; ++Index )
    {
        UObject * Row = MakeSampleRow( Index );
        if ( !Row )
        {

            break;
        }

        Rows.Add( Row );
    }

    return Rows;
}

UObject * USmartTableDemoPage::CopySampleRow( UObject * Source )
{
    if ( !Source )
    {
        return nullptr;
    }

    UObject * Item = NewObject< UObject >( this, Source->GetClass() );

    for ( const FName & Field : StationFields() )
    {
        Write( *Item, *Field.ToString(), Read( *Source, *Field.ToString() ) );
    }

    return Item;
}
