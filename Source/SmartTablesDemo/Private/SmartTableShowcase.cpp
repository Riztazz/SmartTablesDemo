#include "SmartTableShowcase.h"
#include "SmartTableDemoConstants.h"

#include "UObject/ConstructorHelpers.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Logging/StructuredLog.h"
#include "Misc/AssertionMacros.h"
#include "SmartTable.h"
#include "SmartTableCellDragDropOp.h"
#include "SmartTableLog.h"
#include "SmartTableShowcaseCells.h"
#include "SmartTableShowcaseModels.h"
#include "SmartTableStyle.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "SmartTablesDemo"

namespace
{
    FSlateBrush FlatBrush( const FLinearColor & Colour )
    {
        FSlateBrush Brush;
        Brush.DrawAs    = ESlateBrushDrawType::Image;
        Brush.TintColor = FSlateColor( Colour );

        return Brush;
    }

    FTextBlockStyle TextStyle( const FLinearColor & Colour, int32 Size, const FName & Typeface = TEXT( "Regular" ) )
    {
        FTextBlockStyle Style = FTextBlockStyle::GetDefault();
        Style.SetFont( FCoreStyle::GetDefaultFontStyle( Typeface, Size ) );
        Style.SetColorAndOpacity( FSlateColor( Colour ) );

        return Style;
    }

    FSmartTableColumn Column( FName Id, const FText & Header, ESmartTableColumnSizing Sizing, float Width, EHorizontalAlignment Align = HAlign_Left )
    {
        FSmartTableColumn Result;
        Result.ColumnId = Id;
        Result.Header   = Header;
        Result.Sizing   = Sizing;
        Result.Width    = Width;
        Result.HAlign   = Align;

        return Result;
    }
}
void USmartTableShowcaseScreen::NativeOnInitialized()
{
    Super::NativeOnInitialized();
}

void USmartTableShowcaseScreen::BuildScreen()
{
    checkf( WidgetTree, TEXT( "Showcase screen '%s' has no widget tree - it was not made through CreateWidget" ), *GetName() );

    if ( bScreenBuilt )
    {
        UE_LOGFMT( LogSmartTables, Verbose, "Showcase screen '{Screen}' is already built.", GetName() );
        return;
    }

    bScreenBuilt = true;

    checkf( WidgetTree->RootWidget, TEXT( "Showcase screen '%s' has no authored layout - its Widget Blueprint must have a root widget" ), *GetName() );

    ConfigureScreen();
}

USmartTableShowcaseScreen::FPalette USmartTableShowcaseScreen::PaletteFromStyle( const USmartTableStyle & InStyle ) const
{
    FPalette Palette;

    Palette.Backdrop = InStyle.BackgroundBrush.TintColor.GetSpecifiedColor();
    Palette.Panel    = InStyle.HeaderStyle.BackgroundBrush.TintColor.GetSpecifiedColor();
    Palette.RowEven  = InStyle.RowStyle.EvenRowBackgroundBrush.TintColor.GetSpecifiedColor();
    Palette.RowOdd   = InStyle.RowStyle.OddRowBackgroundBrush.TintColor.GetSpecifiedColor();
    Palette.Hover    = InStyle.RowStyle.EvenRowBackgroundHoveredBrush.TintColor.GetSpecifiedColor();
    Palette.Selected = InStyle.RowStyle.ActiveBrush.TintColor.GetSpecifiedColor();
    Palette.Text     = InStyle.CellTextStyle.ColorAndOpacity.GetSpecifiedColor();
    Palette.Muted    = InStyle.HeaderTextStyle.ColorAndOpacity.GetSpecifiedColor();
    Palette.Accent   = InStyle.InsertMarkerBrush.TintColor.GetSpecifiedColor();
    Palette.Rule     = InStyle.ColumnDividerBrush.TintColor.GetSpecifiedColor();

    return Palette;
}

UTextBlock * USmartTableShowcaseScreen::MakeLabel( const FText & Text, int32 Size, const FLinearColor & Colour )
{
    UTextBlock * Label = WidgetTree->ConstructWidget< UTextBlock >( UTextBlock::StaticClass() );
    Label->SetText( Text );
    Label->SetFont( FCoreStyle::GetDefaultFontStyle( TEXT( "Regular" ), Size ) );
    Label->SetColorAndOpacity( FSlateColor( Colour ) );

    return Label;
}

UTextBlock * USmartTableShowcaseScreen::AddHeading( UVerticalBox * Parent, const FText & Text, int32 Size, const FLinearColor & Colour, const FMargin & InPadding )
{
    UTextBlock * Heading = MakeLabel( Text, Size, Colour );
    if ( UVerticalBoxSlot * HeadingSlot = Parent->AddChildToVerticalBox( Heading ) )
    {
        HeadingSlot->SetPadding( InPadding );
    }

    return Heading;
}

USmartTableShowcaseScreen * USmartTableShowcaseScreen::ShowScreenOfClass( UObject * WorldContextObject, TSubclassOf< USmartTableShowcaseScreen > ScreenClass )
{
    if ( !WorldContextObject || !ScreenClass )
    {
        UE_LOGFMT( LogSmartTables, Warning, "ShowScreenOfClass made no screen: {Missing} is not set. Both pins are required - any object in the world will do for the context.", WorldContextObject ? TEXT( "ScreenClass" ) : TEXT( "WorldContextObject" ) );
        return nullptr;
    }

    USmartTableShowcaseScreen * Screen = CreateWidget< USmartTableShowcaseScreen >( WorldContextObject->GetWorld(), ScreenClass );
    if ( !Screen )
    {
        UE_LOGFMT( LogSmartTables, Warning, "ShowScreenOfClass could not create a widget of class {Class}. Use a concrete (non-Abstract) screen class, and pass a context object that belongs to a world.", ScreenClass->GetName() );
        return nullptr;
    }

    Screen->BuildScreen();
    Screen->AddToViewport( 0 );

    return Screen;
}

UBorder * USmartTableShowcaseScreen::MakePanel( const FLinearColor & Colour, const FMargin & InPadding )
{
    UBorder * Panel = WidgetTree->ConstructWidget< UBorder >( UBorder::StaticClass() );
    Panel->SetBrushColor( Colour );
    Panel->SetPadding( InPadding );

    return Panel;
}

USmartTableServerScreen::USmartTableServerScreen( const FObjectInitializer & ObjectInitializer )
    : Super( ObjectInitializer )
{
    const ConstructorHelpers::FObjectFinder< USmartTableStyle > Browser( TEXT( "/Game/Demo/ServerBrowser/ST_Style_Browser" ) );
    ScreenStyle = Browser.Succeeded() ? Browser.Object : nullptr;

    const ConstructorHelpers::FObjectFinder< USmartTableStyle > Feed( TEXT( "/Game/Demo/ServerBrowser/ST_Style_Feed" ) );
    FeedStyle = Feed.Succeeded() ? Feed.Object : nullptr;
}

void USmartTableServerScreen::ConfigureScreen()
{
    checkf( ScreenStyle && FeedStyle, TEXT( "Server screen '%s' is missing a style asset" ), *GetName() );
    checkf( Table && FeedTable, TEXT( "Server screen '%s' must author both tables in its Widget Blueprint" ), *GetName() );

    const FPalette Palette     = PaletteFromStyle( *ScreenStyle );
    const FPalette FeedPalette = PaletteFromStyle( *FeedStyle );

    if ( Backdrop )
    {
        Backdrop->SetBrushColor( Palette.Backdrop );
    }

    if ( TitleText )
    {
        TitleText->SetColorAndOpacity( FSlateColor( Palette.Text ) );
    }

    if ( SubtitleText )
    {
        SubtitleText->SetColorAndOpacity( FSlateColor( Palette.Muted ) );
    }

    if ( FeedPanel )
    {
        FeedPanel->SetBrushColor( FeedPalette.Backdrop );
    }

    if ( DetailPanel )
    {
        DetailPanel->SetBrushColor( Palette.Panel );
    }

    if ( FeedTitle )
    {
        FeedTitle->SetColorAndOpacity( FSlateColor( FeedPalette.Muted ) );
    }

    if ( CounterText )
    {
        CounterText->SetColorAndOpacity( FSlateColor( FeedPalette.Accent ) );
    }

    if ( FilterBox )
    {
        FEditableTextBoxStyle FilterStyle = FilterBox->GetWidgetStyle();
        FilterStyle.TextStyle             = TextStyle( Palette.Text, 12 );
        FilterStyle.SetBackgroundImageNormal( FlatBrush( Palette.RowEven ) );
        FilterStyle.SetBackgroundImageHovered( FlatBrush( Palette.Hover ) );
        FilterStyle.SetBackgroundImageFocused( FlatBrush( Palette.Hover ) );
        FilterStyle.SetBackgroundImageReadOnly( FlatBrush( Palette.RowEven ) );

        FilterStyle.BackgroundColor        = FSlateColor( FLinearColor::White );
        FilterStyle.ForegroundColor        = FSlateColor( Palette.Text );
        FilterStyle.FocusedForegroundColor = FSlateColor( Palette.Text );
        FilterStyle.Padding                = FMargin( 10.0f, 7.0f );

        FilterBox->SetWidgetStyle( FilterStyle );
        FilterBox->OnTextChanged.AddDynamic( this, &USmartTableServerScreen::HandleFilterChanged );
    }

    Model = NewObject< USmartTableServerModel >( this );
    Model->GenerateServers( 220 );

    Table->OnSelectionChanged.AddDynamic( this, &USmartTableServerScreen::HandleSelectionChanged );

    Table->SetModel( Model );
    Table->SortByColumn( TEXT( "Ping" ), ESmartTableSortMode::Ascending );

    FeedModel = NewObject< USmartTableFeedModel >( this );
    FeedModel->Prime( 40 );

    FeedTable->SetModel( FeedModel );

    ConfigureLoadout( Palette );

    ShowDetail( INDEX_NONE );

    PushEntry();
}

void USmartTableServerScreen::HandleSelectionChanged( const TArray< int32 > & SelectedRows )
{
    ShowDetail( SelectedRows.IsEmpty() ? INDEX_NONE : SelectedRows[ 0 ] );
}

void USmartTableServerScreen::ShowDetail( int32 NaturalRow )
{
    checkf( DetailBox && Model, TEXT( "The server screen showed a detail before ComposeScreen built it" ) );

    DetailBox->ClearChildren();

    const FLinearColor Text( 0.92f, 0.88f, 0.98f, 1.0f );
    const FLinearColor Muted( 0.52f, 0.44f, 0.68f, 1.0f );
    const FLinearColor Accent( 0.98f, 0.28f, 0.62f, 1.0f );

    if ( NaturalRow == INDEX_NONE )
    {
        AddHeading( DetailBox, LOCTEXT( "ServerNone", "No server selected" ), 16, Muted, FMargin( 0.0f ) );
        AddHeading( DetailBox, LOCTEXT( "ServerNoneHint", "Pick one from the list to see what is on it." ), 12, Muted, FMargin( 0.0f, 6.0f, 0.0f, 0.0f ) );

        return;
    }

    AddHeading( DetailBox, Model->GetServerName( NaturalRow ), 20, Text, FMargin( 0.0f, 0.0f, 0.0f, 4.0f ) );
    AddHeading( DetailBox, Model->IsLocked( NaturalRow ) ? LOCTEXT( "ServerLocked", "password required" ) : LOCTEXT( "ServerOpen", "open to everyone" ), 12, Muted, FMargin( 0.0f, 0.0f, 0.0f, 16.0f ) );

    const TCHAR * Fields[] = { TEXT( "Mode" ), TEXT( "Map" ), TEXT( "Players" ), TEXT( "Ping" ) };
    const FText Labels[]   = { LOCTEXT( "ServerFieldMode", "MODE" ), LOCTEXT( "ServerFieldMap", "MAP" ), LOCTEXT( "ServerFieldPlayers", "PLAYERS" ), LOCTEXT( "ServerFieldPing", "PING" ) };

    for ( int32 Pair = 0; Pair < UE_ARRAY_COUNT( Fields ); Pair += 2 )
    {
        UHorizontalBox * Row = WidgetTree->ConstructWidget< UHorizontalBox >( UHorizontalBox::StaticClass() );
        for ( int32 Index = Pair; Index < Pair + 2; ++Index )
        {
            UVerticalBox * Field = WidgetTree->ConstructWidget< UVerticalBox >( UVerticalBox::StaticClass() );
            AddHeading( Field, Labels[ Index ], 10, Muted, FMargin( 0.0f, 0.0f, 0.0f, 2.0f ) );
            AddHeading( Field, Model->GetCellText( NaturalRow, FName( Fields[ Index ] ) ), 15, Text, FMargin( 0.0f ) );
            if ( UHorizontalBoxSlot * FieldSlot = Row->AddChildToHorizontalBox( Field ) )
            {
                FieldSlot->SetSize( FSlateChildSize( ESlateSizeRule::Fill ) );
            }
        }

        if ( UVerticalBoxSlot * RowSlot = DetailBox->AddChildToVerticalBox( Row ) )
        {
            RowSlot->SetPadding( FMargin( 0.0f, 0.0f, 0.0f, 12.0f ) );
        }
    }
}

void USmartTableServerScreen::ConfigureLoadout( const FPalette & Palette )
{
    if ( !LoadoutTable )
    {
        return;
    }

    if ( LoadoutTitle )
    {
        LoadoutTitle->SetColorAndOpacity( FSlateColor( Palette.Muted ) );
    }

    LoadoutModel = NewObject< USmartTableInventoryModel >( this );

    LoadoutModel->GenerateBag( LoadoutColumns, LoadoutRows );

    LoadoutTable->OnCellDropped.AddDynamic( this, &USmartTableServerScreen::HandleSlotDropped );

    TArray< FSmartTableColumn > SlotColumns;
    for ( const FName SlotId : LoadoutModel->GetSlotColumnIds() )
    {
        FSmartTableColumn SlotColumn = Column( SlotId, FText::GetEmpty(), ESmartTableColumnSizing::Fixed, LoadoutSlotSize, HAlign_Center );
        SlotColumn.CellClass         = USmartTableSlotCell::StaticClass();

        SlotColumn.bSortable   = false;
        SlotColumn.bResizable  = false;
        SlotColumn.bHideable   = false;
        SlotColumn.CellPadding = SmartTableDemo::PositionCellPadding();

        SlotColumns.Add( SlotColumn );
    }

    LoadoutTable->SetColumns( SlotColumns );
    LoadoutTable->SetModel( LoadoutModel );
}

void USmartTableServerScreen::HandleSlotDropped( USmartTableCellDragDropOp * Payload, int32 TargetRow, FName TargetColumnId )
{
    if ( !Payload || !LoadoutModel )
    {
        return;
    }

    if ( Payload->SourceRow == TargetRow && Payload->SourceColumnId == TargetColumnId )
    {
        return;
    }

    LoadoutModel->SwapSlots( Payload->SourceRow, Payload->SourceColumnId, TargetRow, TargetColumnId );
}

void USmartTableServerScreen::BurstFeed( int32 Count )
{
    if ( !FeedModel || !FeedTable || Count <= 0 )
    {
        return;
    }

    for ( int32 Index = 0; Index < Count; ++Index )
    {
        FeedModel->Append();
    }

    for ( int32 Index = 0; Index < Count; ++Index )
    {
        FeedModel->DropOldest();
    }

    UE_LOGFMT( LogSmartTables, Verbose, "Feed burst: {Count} arrived and {Count} left in one frame, leaving {Rows} row(s).", Count, Count, FeedModel->GetNumPresentedRows() );
}

void USmartTableServerScreen::PushEntry()
{
    checkf( FeedModel && FeedTable && CounterText, TEXT( "The server screen pushed a feed entry before ComposeScreen built it" ) );

    FeedModel->Append();

    if ( FeedModel->GetNumPresentedRows() > MaxFeedRows )
    {
        FeedModel->DropOldest();
    }

    CounterText->SetText( FText::Format( LOCTEXT( "ServerFeedCounter", "{0} entries   -   {1} at warning or above" ), FText::AsNumber( FeedModel->GetNumPresentedRows() ), FText::AsNumber( FeedModel->GetCountAtOrAbove( ESmartTableFeedSeverity::Warning ) ) ) );

    UWorld * World = GetWorld();
    if ( !World )
    {
        UE_LOGFMT( LogSmartTables, Verbose, "The server screen has no world, so no further feed entries are scheduled." );
        return;
    }

    const float Delay = FeedIntervalSeconds + ( FeedModel->GetNumPresentedRows() % 5 ) * 0.22f;

    World->GetTimerManager().SetTimer( FeedTimer, FTimerDelegate::CreateUObject( this, &USmartTableServerScreen::PushEntry ), Delay, false );
}

void USmartTableServerScreen::NativeDestruct()
{
    if ( UWorld * World = GetWorld() )
    {
        World->GetTimerManager().ClearTimer( FeedTimer );
    }

    Super::NativeDestruct();
}

void USmartTableServerScreen::HandleFilterChanged( const FText & Text )
{
    checkf( Table, TEXT( "The server screen filtered before ComposeScreen built its table" ) );

    Table->SetFilterText( Text );
}

USmartTablePairsScreen::USmartTablePairsScreen( const FObjectInitializer & ObjectInitializer )
    : Super( ObjectInitializer )
{
    const ConstructorHelpers::FObjectFinder< USmartTableStyle > Pairs( TEXT( "/Game/Demo/Pairs/ST_Style_Pairs" ) );
    ScreenStyle = Pairs.Succeeded() ? Pairs.Object : nullptr;
}

void USmartTablePairsScreen::ConfigureScreen()
{
    checkf( ScreenStyle, TEXT( "Showcase screen '%s' has no ScreenStyle asset" ), *GetName() );
    checkf( Table, TEXT( "Pairs screen '%s' has no Table - its Widget Blueprint must author one" ), *GetName() );

    const FPalette Palette = PaletteFromStyle( *ScreenStyle );

    if ( Backdrop )
    {
        Backdrop->SetBrushColor( Palette.Backdrop );
    }

    if ( TitleText )
    {
        TitleText->SetColorAndOpacity( FSlateColor( Palette.Text ) );
    }

    if ( SubtitleText )
    {
        SubtitleText->SetColorAndOpacity( FSlateColor( Palette.Muted ) );
    }

    if ( ScoreText )
    {
        ScoreText->SetColorAndOpacity( FSlateColor( Palette.Accent ) );
    }

    Model = NewObject< USmartTablePairsModel >( this );
    Model->OnTurn.AddDynamic( this, &USmartTablePairsScreen::HandleTurn );
    Model->Deal( BoardColumns, BoardRows );

    UClass * CardClass = CardCellClass.LoadSynchronous();
    if ( !CardClass )
    {
        UE_LOGFMT( LogSmartTables, Warning, "The pairs board has no CardCellClass, so its cards cannot animate. Set it on the '{Screen}' Blueprint to WBP_Card.", GetName() );
    }

    TArray< FSmartTableColumn > CardColumns;
    for ( const FName CardId : Model->GetCardColumnIds() )
    {
        FSmartTableColumn CardColumn = Column( CardId, FText::GetEmpty(), ESmartTableColumnSizing::Fixed, CardSize, HAlign_Center );
        CardColumn.CellClass         = CardClass;

        CardColumn.bSortable   = false;
        CardColumn.bResizable  = false;
        CardColumn.bHideable   = false;
        CardColumn.CellPadding = SmartTableDemo::PositionCellPadding();

        CardColumns.Add( CardColumn );
    }

    Table->SetColumns( CardColumns );
    Table->SetModel( Model );

    ShowScore();
}

void USmartTablePairsScreen::FlipAt( int32 Row, int32 Column )
{
    if ( !Model )
    {
        return;
    }

    const TArray< FName > CardColumns = Model->GetCardColumnIds();
    if ( !CardColumns.IsValidIndex( Column - 1 ) )
    {
        UE_LOGFMT( LogSmartTables, Warning, "FlipAt({Row}, {Column}) refused: the board is {Wide} column(s) wide.", Row, Column, CardColumns.Num() );
        return;
    }

    Model->FlipCard( Row - 1, CardColumns[ Column - 1 ] );
}

void USmartTablePairsScreen::HandleTurn( ESmartTablePairsTurn Turn )
{
    ShowScore();

    if ( Turn != ESmartTablePairsTurn::Missed )
    {
        return;
    }

    UWorld * World = GetWorld();
    if ( !World )
    {
        return;
    }

    World->GetTimerManager().SetTimer( TurnBackTimer, FTimerDelegate::CreateUObject( Model.Get(), &USmartTablePairsModel::TurnBack ), MissSeconds, false );
}

void USmartTablePairsScreen::ShowScore()
{
    if ( !ScoreText || !Model )
    {
        return;
    }

    ScoreText->SetText(
        Model->IsComplete() ? FText::Format( LOCTEXT( "PairsDone", "All {0} pairs in {1} moves." ), FText::AsNumber( Model->GetPairsTotal() ), FText::AsNumber( Model->GetMoves() ) ) : FText::Format( LOCTEXT( "PairsScore", "{0} of {1} pairs   -   {2} moves" ), FText::AsNumber( Model->GetPairsFound() ), FText::AsNumber( Model->GetPairsTotal() ), FText::AsNumber( Model->GetMoves() ) ) );
}

#undef LOCTEXT_NAMESPACE
