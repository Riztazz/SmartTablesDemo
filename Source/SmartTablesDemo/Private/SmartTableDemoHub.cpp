#include "SmartTableDemoHub.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Logging/StructuredLog.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/AssertionMacros.h"
#include "SmartTable.h"
#include "SmartTableAsyncDemoModel.h"
#include "SmartTableDataTablePage.h"
#include "SmartTableDemoMenu.h"
#include "SmartTableDemoPage.h"
#include "SmartTableDemoRow.h"
#include "SmartTableLog.h"
#include "SmartTableModel.h"
#include "SmartTableOffscreenStage.h"
#include "SmartTableRecordFormatPage.h"
#include "SmartTableShowcase.h"
#include "SmartTableStationViewModel.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "SmartTablesDemo"

void USmartTableDemoHub::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    BuildLayout();
}

void USmartTableDemoHub::BuildLayout()
{
    checkf( Pages && MenuTable && TableHost && TitleText && SelectionBanner && ScreenHost && BackButton && ScreenBackButton, TEXT( "BuildLayout ran without the widgets the Blueprint is required to bind" ) );

    FillMenuTable();

    FScriptDelegate BackHandler;
    BackHandler.BindUFunction( this, GET_FUNCTION_NAME_CHECKED( USmartTableDemoHub, ShowMenu ) );
    BackButton->OnClicked.Add( BackHandler );
    ScreenBackButton->OnClicked.Add( BackHandler );

    Pages->SetActiveWidgetIndex( 0 );
}

bool USmartTableDemoHub::OpenDemoPage( const TSoftClassPtr< USmartTableDemoPage > & PageClass )
{
    UClass * Loaded = PageClass.LoadSynchronous();
    if ( !Loaded )
    {
        UE_LOGFMT( LogSmartTables, Warning, "A demo page is not set, so that demo cannot open. Set it on the demo hub Blueprint to a Widget Blueprint deriving from SmartTableDemoPage." );

        return false;
    }

    TakeDownCurrent();

    CurrentPage = CreateWidget< USmartTableDemoPage >( this, Loaded );
    checkf( CurrentPage, TEXT( "CreateWidget returned nothing for demo page %s" ), *Loaded->GetName() );

    Table = CurrentPage->GetTable();
    checkf( Table, TEXT( "Demo page %s compiled without its BindWidget table" ), *Loaded->GetName() );

    TableHost->AddChild( CurrentPage );

    if ( PageActionsHost && CurrentPage->GetPageActions() )
    {
        PageActionsHost->SetContent( CurrentPage->GetPageActions() );
    }

    FScriptDelegate SelectionHandler;
    SelectionHandler.BindUFunction( this, GET_FUNCTION_NAME_CHECKED( USmartTableDemoHub, HandleSelectionForBanner ) );
    Table->OnSelectionChanged.Add( SelectionHandler );

    return true;
}

void USmartTableDemoHub::FillMenuTable()
{
    FScriptDelegate Played;
    Played.BindUFunction( this, GET_FUNCTION_NAME_CHECKED( USmartTableDemoHub, HandleMenuRowChosen ) );
    MenuTable->OnCellValueChanged.Add( Played );

    struct FDemoEntry
    {
        const TCHAR * Title;
        const TCHAR * Blurb;

        bool bListed = true;
    };

    const FDemoEntry Demos[] = {
        { TEXT( "Items" ), TEXT( "A UObject with variables on it. Point the table at the class and it reads the properties off it. Set up in the Details panel or from C++." ) },
        { TEXT( "DataTable" ), TEXT( "Rows out of a DataTable asset. The row struct is typed, so cells come out as number boxes, sliders and checkboxes." ) },
        { TEXT( "CSV file" ), TEXT( "Reads a .csv off disk. No types in a text file, so every field is a text box. Save writes it back." ) },
        { TEXT( "JSON file" ), TEXT( "Same thing with .json. Edits stay in memory until you save." ) },
        { TEXT( "One million rows" ), TEXT( "Async model and virtualised rendering of a million rows. Sorting runs off the game thread." ) },
        { TEXT( "Server browser" ), TEXT( "A table used in an actual game screen. Resize, reorder, two level sort and filtering, all off the header." ) },
        { TEXT( "Pairs" ), TEXT( "A memory game. Every column is a position and every cell is a card that animates itself in Sequencer." ) },
        { TEXT( "Keys without Enhanced Input" ), TEXT( "Items demo with keyboard input through OnKeyDown. No Enhanced Input." ) },
        { TEXT( "Off screen, virtual pointer" ), TEXT( "The Items table drawn off screen at a draw scale of 1.173 and clicked through a virtual Slate user, which is what a diegetic screen does. Header clicks, resize edges and menus all behave differently in here." ), false },
        { TEXT( "Items with MVVM" ), TEXT( "The Items demo with a viewmodel for each row. Every cell names an Item Setter and binds its widgets in its Widget Blueprint, with no graph. A feed moves one pressure a few times a second, and only that cell draws again." ) },
    };

    TArray< UObject * > Items;
    for ( int32 Index = 0; Index < UE_ARRAY_COUNT( Demos ); ++Index )
    {
        const FDemoEntry & Demo = Demos[ Index ];
        if ( !Demo.bListed )
        {
            continue;
        }

        USmartTableDemoMenuItem * Item = NewObject< USmartTableDemoMenuItem >( this );
        Item->Number                   = FString::FromInt( Items.Num() + 1 );
        Item->Title                    = Demo.Title;
        Item->Blurb                    = Demo.Blurb;
        Item->DemoIndex                = Index;

        Items.Add( Item );
    }

    MenuTable->SetItems( Items );
}

void USmartTableDemoHub::BeginDemo( const FText & Title )
{
    checkf( TitleText && Pages, TEXT( "BeginDemo ran before BuildLayout wired the demo page" ) );

    UE_LOGFMT( LogSmartTables, Verbose, "Demo '{Demo}' starting.", GetNameSafe( CurrentPage ) );

    TitleText->SetText( Title );

    TitleText->SetVisibility( ESlateVisibility::HitTestInvisible );

    HandleSelectionForBanner( Table->GetSelectedRows() );

    Pages->SetActiveWidgetIndex( 1 );
}

void USmartTableDemoHub::ShowItemDemo()
{
    BuildItemDemo( ItemsPage, LOCTEXT( "ItemsHeading", "Items - UObject based" ) );
}

void USmartTableDemoHub::ShowKeyboardInputDemo()
{
    BuildItemDemo( KeyboardInputPage, LOCTEXT( "KeyboardHeading", "Keys - no Enhanced Input" ) );
}

void USmartTableDemoHub::ShowMvvmItemDemo()
{
    BuildItemDemo( MvvmItemsPage, LOCTEXT( "MvvmItemsHeading", "Items - MVVM viewmodels" ) );

    if ( !CurrentPage || StationFeedSeconds <= 0.0f )
    {
        return;
    }

    UWorld * World = GetWorld();
    checkf( World, TEXT( "The demo hub is on screen with no world" ) );

    World->GetTimerManager().SetTimer( StationFeed, this, &USmartTableDemoHub::NudgeStation, StationFeedSeconds, true );
}

void USmartTableDemoHub::NudgeStation()
{
    USmartTableModel * Model             = Table ? Table->GetModel() : nullptr;
    const int32 RowCount                 = Model ? Model->GetNumRows() : 0;
    const int32 Row                      = RowCount > 0 ? FMath::RandHelper( RowCount ) : INDEX_NONE;
    USmartTableStationViewModel * Nudged = Row != INDEX_NONE ? Cast< USmartTableStationViewModel >( Model->GetRowItem( Row ) ) : nullptr;

    if ( !Nudged )
    {
        UE_LOGFMT( LogSmartTables, Warning, "The station feed stops. Page {Page} holds no Smart Table Station Viewmodel rows. Set its Sample Row Class to that class.", GetNameSafe( CurrentPage ) );

        GetWorld()->GetTimerManager().ClearTimer( StationFeed );

        return;
    }

    Nudged->SetPressureHpa( Nudged->PressureHpa + FMath::RoundToDouble( FMath::FRandRange( -4.0, 4.0 ) ) );

    Model->NotifyRowChanged( Row );
}

void USmartTableDemoHub::BuildItemDemo( const TSoftClassPtr< USmartTableDemoPage > & PageClass, const FText & Title )
{
    if ( !OpenDemoPage( PageClass ) )
    {
        return;
    }

    BeginDemo( Title );
}

void USmartTableDemoHub::ShowDataTableDemo()
{
    if ( !OpenDemoPage( DataTablePage ) )
    {
        return;
    }

    USmartTableDataTablePage * Page = Cast< USmartTableDataTablePage >( CurrentPage );
    if ( !Page )
    {
        UE_LOGFMT( LogSmartTables, Warning, "The DataTable demo did not open: set DataTablePage on the demo hub Blueprint to a Widget Blueprint deriving from SmartTableDataTablePage." );
        return;
    }

    Page->BeginDataTable( DemoDataTable.LoadSynchronous() );

    BeginDemo( LOCTEXT( "DataTableHeading", "DataTable - typed row struct" ) );
}

void USmartTableDemoHub::ShowCsvDemo()
{
    OpenRecordFormatDemo( CsvPage, LOCTEXT( "CsvHeading", "CSV - untyped text file" ) );
}

void USmartTableDemoHub::ShowJsonDemo()
{
    OpenRecordFormatDemo( JsonPage, LOCTEXT( "JsonHeading", "JSON - untyped text file" ) );
}

void USmartTableDemoHub::OpenRecordFormatDemo( const TSoftClassPtr< USmartTableRecordFormatPage > & PageClass, const FText & Title )
{
    if ( !OpenDemoPage( PageClass ) )
    {
        return;
    }

    USmartTableRecordFormatPage * Page = Cast< USmartTableRecordFormatPage >( CurrentPage );
    if ( !Page )
    {
        UE_LOGFMT( LogSmartTables, Warning, "A file-format demo did not open: its page must be a Widget Blueprint deriving from SmartTableRecordFormatPage." );
        return;
    }

    Page->BeginRecordFormat();

    BeginDemo( Title );
}

void USmartTableDemoHub::ShowHugeDemo()
{
    BuildAsyncDemo( MillionPage, HugeDemoRowCount, LOCTEXT( "HugeHeading", "One million rows - async model" ) );
}

void USmartTableDemoHub::BuildAsyncDemo( const TSoftClassPtr< USmartTableDemoPage > & PageClass, int32 RowCount, const FText & Title )
{
    if ( !OpenDemoPage( PageClass ) )
    {
        return;
    }

    if ( !AsyncModel )
    {
        AsyncModel = NewObject< USmartTableAsyncDemoModel >( this );
    }

    AsyncModel->GenerateRows( RowCount );
    AsyncModel->UseAsyncSorting( AsyncDemoLatencySeconds );

    AsyncModel->UseStatusRowColors( false );

    Table->SetModel( AsyncModel );

    BeginDemo( Title );
}

void USmartTableDemoHub::OpenDemoByIndex( int32 DemoIndex )
{
    switch ( DemoIndex )
    {
        case 0:
            ShowItemDemo();
            break;
        case 1:
            ShowDataTableDemo();
            break;
        case 2:
            ShowCsvDemo();
            break;
        case 3:
            ShowJsonDemo();
            break;
        case 4:
            ShowHugeDemo();
            break;
        case 5:
            ShowServerScreen();
            break;
        case 6:
            ShowPairsScreen();
            break;
        case 7:
            ShowKeyboardInputDemo();
            break;
        case 8:
            ShowOffscreenDemo();
            break;
        case 9:
            ShowMvvmItemDemo();
            break;

        default:
            UE_LOGFMT( LogSmartTables, Warning, "Menu row {Index} has no demo behind it. Add a case for it in OpenDemoByIndex, or take the row out of BuildMenuTable.", DemoIndex );
            break;
    }
}

void USmartTableDemoHub::HandleMenuRowChosen( UObject * Item, int32 NaturalRow, FName ColumnId, float Value )
{
    if ( const USmartTableDemoMenuItem * MenuItem = Cast< USmartTableDemoMenuItem >( Item ) )
    {
        OpenDemoByIndex( MenuItem->DemoIndex );
    }
}

void USmartTableDemoHub::HandleSelectionForBanner( const TArray< int32 > & SelectedRows )
{
    checkf( SelectionBanner, TEXT( "The selection banner is missing; BuildLayout did not run before its own handler fired" ) );

    SelectionBanner->SetText( SelectedRows.IsEmpty() ? LOCTEXT( "NothingSelected", "nothing selected" ) : FText::Format( LOCTEXT( "SomeSelected", "{0} selected - first is row {1}" ), FText::AsNumber( SelectedRows.Num() ), FText::AsNumber( SelectedRows[ 0 ] + 1 ) ) );
}

void USmartTableDemoHub::ShowServerScreen()
{
    UClass * Authored = ServerBrowserScreen.LoadSynchronous();
    if ( !Authored )
    {
        UE_LOGFMT( LogSmartTables, Warning, "The server browser did not open: set ServerBrowserScreen on the demo hub Blueprint to WBP_Screen_ServerBrowser." );
        return;
    }

    ShowScreen( Authored );
}

void USmartTableDemoHub::ShowPairsScreen()
{
    UClass * Authored = PairsScreen.LoadSynchronous();
    if ( !Authored )
    {
        UE_LOGFMT( LogSmartTables, Warning, "The pairs board did not open: set PairsScreen on the demo hub Blueprint to WBP_Screen_Pairs." );
        return;
    }

    ShowScreen( Authored );
}

void USmartTableDemoHub::ShowScreen( TSubclassOf< USmartTableShowcaseScreen > ScreenClass )
{
    checkf( ScreenClass, TEXT( "A showcase screen was asked for with no class" ) );

    TakeDownCurrent();

    CurrentScreen = CreateWidget< USmartTableShowcaseScreen >( this, ScreenClass );

    checkf( CurrentScreen, TEXT( "Could not create showcase screen '%s'" ), *ScreenClass->GetName() );

    UE_LOGFMT( LogSmartTables, Verbose, "Showing screen {Screen}.", ScreenClass->GetName() );

    CurrentScreen->BuildScreen();

    ScreenHost->AddChild( CurrentScreen );

    Pages->SetActiveWidgetIndex( 2 );
}

void USmartTableDemoHub::ShowOffscreenDemo()
{

    UClass * Loaded = ItemsPage.LoadSynchronous();
    if ( !Loaded )
    {
        UE_LOGFMT( LogSmartTables, Warning, "The offscreen demo opened nothing. Set ItemsPage on the demo hub Blueprint to a Widget Blueprint deriving from SmartTableDemoPage - the offscreen demo draws that page." );

        return;
    }

    TakeDownCurrent();

    UWorld * World = GetWorld();
    checkf( World, TEXT( "The demo hub is on screen with no world" ) );

    OffscreenStage = World->SpawnActorDeferred< ASmartTableOffscreenStage >( ASmartTableOffscreenStage::StaticClass(), FTransform::Identity );
    if ( !OffscreenStage )
    {
        UE_LOGFMT( LogSmartTables, Warning, "The offscreen demo opened nothing. The world refused to spawn its stage actor." );

        return;
    }

    OffscreenStage->PageClass = Loaded;

    OffscreenStage->RenderScale = 1.173f;

    OffscreenStage->DisplayHost = TableHost;

    OffscreenStage->FinishSpawning( FTransform::Identity );

    if ( PageActionsHost )
    {
        if ( USmartTableDemoPage * Drawn = Cast< USmartTableDemoPage >( OffscreenStage->GetPage() ) )
        {
            PageActionsHost->SetContent( Drawn->GetPageActions() );
        }
    }

    TitleText->SetText( LOCTEXT( "OffscreenHeading", "Off screen - virtual pointer" ) );
    TitleText->SetVisibility( ESlateVisibility::HitTestInvisible );

    HandleSelectionForBanner( TArray< int32 >() );

    UE_LOGFMT( LogSmartTables, Verbose, "Showing the offscreen demo at scale {Scale}.", OffscreenStage->GetRenderScale() );

    Pages->SetActiveWidgetIndex( 1 );
}

void USmartTableDemoHub::ShowMenu()
{
    UE_LOGFMT( LogSmartTables, Verbose, "Back to the demo menu; whatever was open is taken down." );

    Pages->SetActiveWidgetIndex( 0 );
    TakeDownCurrent();
}

void USmartTableDemoHub::TakeDownCurrent()
{
    if ( StationFeed.IsValid() )
    {
        GetWorld()->GetTimerManager().ClearTimer( StationFeed );
    }

    if ( CurrentPage )
    {
        Table->SetModel( nullptr );

        if ( PageActionsHost )
        {
            PageActionsHost->ClearChildren();
        }

        TableHost->ClearChildren();
        CurrentPage = nullptr;
        Table       = nullptr;
    }

    if ( CurrentScreen )
    {
        ScreenHost->ClearChildren();
        CurrentScreen = nullptr;
    }

    if ( OffscreenStage )
    {
        OffscreenStage->Destroy();
        OffscreenStage = nullptr;

        TableHost->ClearChildren();

        if ( PageActionsHost )
        {
            PageActionsHost->ClearChildren();
        }
    }

    AsyncModel = nullptr;
}

#undef LOCTEXT_NAMESPACE
