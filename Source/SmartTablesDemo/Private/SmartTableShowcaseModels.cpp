#include "SmartTableShowcaseModels.h"
#include "SmartTableDemoConstants.h"

#include "Internationalization/Text.h"
#include "Logging/StructuredLog.h"
#include "SmartTableLog.h"

#define LOCTEXT_NAMESPACE "SmartTables"

namespace
{
    int32 Spread( int32 Index, int32 Salt, int32 Range )
    {
        const int32 Mixed = ( Index * 2654435761u + Salt * 40503u ) & 0x7FFFFFFF;

        return Range > 0 ? Mixed % Range : 0;
    }
}
namespace
{
    FLinearColor SeverityColour( ESmartTableFeedSeverity Severity )
    {
        switch ( Severity )
        {
            case ESmartTableFeedSeverity::Critical:
                return FLinearColor( 0.95f, 0.32f, 0.30f, 1.0f );

            case ESmartTableFeedSeverity::Warning:
                return FLinearColor( 0.96f, 0.74f, 0.26f, 1.0f );

            case ESmartTableFeedSeverity::Info:
                return FLinearColor( 0.40f, 0.80f, 0.95f, 1.0f );

            default:
                return FLinearColor( 0.45f, 0.50f, 0.54f, 1.0f );
        }
    }

    FText SeverityName( ESmartTableFeedSeverity Severity )
    {
        switch ( Severity )
        {
            case ESmartTableFeedSeverity::Critical:
                return LOCTEXT( "SevCritical", "CRIT" );

            case ESmartTableFeedSeverity::Warning:
                return LOCTEXT( "SevWarning", "WARN" );

            case ESmartTableFeedSeverity::Info:
                return LOCTEXT( "SevInfo", "INFO" );

            default:
                return LOCTEXT( "SevTrace", "TRACE" );
        }
    }

    const TCHAR * FeedSources[] = { TEXT( "MATCH" ), TEXT( "LOBBY" ), TEXT( "AUTH" ), TEXT( "NET" ), TEXT( "HOST" ), TEXT( "QUEUE" ), TEXT( "ANTI-CH" ), TEXT( "REGION" ) };

    const TCHAR * FeedMessages[] = {
        TEXT( "player joined - 14/24 on Ridgeline" ),
        TEXT( "match ended, rotating to next map" ),
        TEXT( "host migrated, 2 clients reconnecting" ),
        TEXT( "packet loss 4% on eu-west, watching" ),
        TEXT( "region eu-central back in rotation" ),
        TEXT( "queue depth 38, average wait 24s" ),
        TEXT( "kick: client failed integrity check" ),
        TEXT( "lobby filled, starting in 10s" ),
        TEXT( "tick rate dropped to 48, shedding bots" ),
        TEXT( "server restarted after config reload" ),
    };
}
void USmartTableFeedModel::Prime( int32 NumEntries )
{
    UE_LOGFMT( LogSmartTables, Verbose, "Feed model priming a backlog of {Count} entr(ies).", NumEntries );

    Entries.Reset( NumEntries );
    Clock = 0;

    for ( int32 Index = 0; Index < NumEntries; ++Index )
    {
        Append();
    }
}

void USmartTableFeedModel::Append()
{
    const int32 Index = Entries.Num();

    FEntry Entry;
    Entry.Tick = ++Clock;

    Entry.Source  = FeedSources[ Spread( Entry.Tick, 3, UE_ARRAY_COUNT( FeedSources ) ) ];
    Entry.Message = FeedMessages[ Spread( Entry.Tick, 7, UE_ARRAY_COUNT( FeedMessages ) ) ];

    const int32 Roll = Spread( Entry.Tick, 11, 100 );
    Entry.Severity   = Roll > 94 ? ESmartTableFeedSeverity::Critical : Roll > 80 ? ESmartTableFeedSeverity::Warning : Roll > 35 ? ESmartTableFeedSeverity::Info : ESmartTableFeedSeverity::Trace;

    Entries.Add( MoveTemp( Entry ) );

    UE_LOGFMT( LogSmartTables, VeryVerbose, "Feed entry {Index} arrived at severity {Severity}.", Index, static_cast< int32 >( Entries.Last().Severity ) );

    NotifyRowsAdded( { Index } );
}

void USmartTableFeedModel::DropOldest()
{
    if ( Entries.IsEmpty() )
    {
        return;
    }

    const FName Oldest = GetRowId( 0 );

    Entries.RemoveAt( 0 );

    NotifyRowsRemoved( { Oldest } );
}

int32 USmartTableFeedModel::GetCountAtOrAbove( ESmartTableFeedSeverity Minimum ) const
{
    int32 Count = 0;
    for ( const FEntry & Entry : Entries )
    {
        if ( Entry.Severity >= Minimum )
        {
            ++Count;
        }
    }

    return Count;
}

int32 USmartTableFeedModel::GetNumRows_Implementation()
{
    return Entries.Num();
}

FText USmartTableFeedModel::GetCellText_Implementation( int32 NaturalRow, FName ColumnId )
{
    if ( !Entries.IsValidIndex( NaturalRow ) )
    {
        return FText::GetEmpty();
    }

    const FEntry & Entry = Entries[ NaturalRow ];

    if ( ColumnId == TEXT( "Time" ) )
    {
        return FText::FromString( FString::Printf( TEXT( "T+%04d" ), Entry.Tick ) );
    }

    if ( ColumnId == TEXT( "Severity" ) )
    {
        return SeverityName( Entry.Severity );
    }

    if ( ColumnId == TEXT( "Entry" ) )
    {
        return FText::FromString( FString::Printf( TEXT( "%s\n%s" ), *Entry.Message, *Entry.Source ) );
    }

    return FText::GetEmpty();
}

FLinearColor USmartTableFeedModel::GetCellColor_Implementation( int32 NaturalRow, FName ColumnId )
{
    if ( !Entries.IsValidIndex( NaturalRow ) )
    {
        return SmartTableDemo::NoColourOpinion();
    }

    if ( ColumnId == TEXT( "Severity" ) || ColumnId == TEXT( "Pip" ) || ColumnId == TEXT( "Entry" ) )
    {
        return SeverityColour( Entries[ NaturalRow ].Severity );
    }

    return SmartTableDemo::NoColourOpinion();
}

FLinearColor USmartTableFeedModel::GetRowColor_Implementation( int32 NaturalRow )
{
    if ( !Entries.IsValidIndex( NaturalRow ) || Entries[ NaturalRow ].Severity < ESmartTableFeedSeverity::Warning )
    {
        return SmartTableDemo::NoColourOpinion();
    }

    return SeverityColour( Entries[ NaturalRow ].Severity ) * FLinearColor( 1.0f, 1.0f, 1.0f, 0.13f );
}

FSmartTableSortKey USmartTableFeedModel::GetCellSortKey_Implementation( int32 NaturalRow, FName ColumnId )
{
    if ( !Entries.IsValidIndex( NaturalRow ) )
    {
        return FSmartTableSortKey::MakeEmpty();
    }

    const FEntry & Entry = Entries[ NaturalRow ];

    if ( ColumnId == TEXT( "Time" ) )
    {
        return FSmartTableSortKey::MakeNumber( Entry.Tick );
    }

    if ( ColumnId == TEXT( "Severity" ) || ColumnId == TEXT( "Pip" ) )
    {
        return FSmartTableSortKey::MakeNumber( static_cast< double >( Entry.Severity ) );
    }

    if ( ColumnId == TEXT( "Entry" ) )
    {
        return FSmartTableSortKey::MakeText( Entry.Message );
    }

    return FSmartTableSortKey::MakeEmpty();
}

FName USmartTableFeedModel::GetRowId_Implementation( int32 NaturalRow )
{
    return Entries.IsValidIndex( NaturalRow ) ? FName( TEXT( "LOG" ), Entries[ NaturalRow ].Tick ) : NAME_None;
}

void USmartTableServerModel::GenerateServers( int32 NumServers )
{
    static const TCHAR * Prefixes[] = { TEXT( "BLACKSITE" ), TEXT( "NEON HARBOUR" ), TEXT( "GRID" ), TEXT( "HOLLOW" ), TEXT( "ZEPHYR" ), TEXT( "SALT FLATS" ), TEXT( "MIDNIGHT" ), TEXT( "OVERPASS" ), TEXT( "KILN" ), TEXT( "VERTIGO" ) };

    static const TCHAR * Regions[] = { TEXT( "EU-WEST" ), TEXT( "EU-NORTH" ), TEXT( "NA-EAST" ), TEXT( "NA-WEST" ), TEXT( "APAC" ), TEXT( "SA-EAST" ) };
    static const TCHAR * Modes[]   = { TEXT( "Extraction" ), TEXT( "Deathmatch" ), TEXT( "Siege" ), TEXT( "Co-op" ), TEXT( "Hardcore" ) };
    static const TCHAR * Maps[]    = { TEXT( "Refinery" ), TEXT( "Undercroft" ), TEXT( "Tidal Works" ), TEXT( "Ash Quarter" ), TEXT( "Cold Storage" ), TEXT( "Spindle" ) };

    Servers.Reset( NumServers );

    for ( int32 Index = 0; Index < NumServers; ++Index )
    {
        FServer Server;

        Server.Name      = FString::Printf( TEXT( "%s #%02d" ), Prefixes[ Index % UE_ARRAY_COUNT( Prefixes ) ], 1 + ( Index / UE_ARRAY_COUNT( Prefixes ) * 7 ) % 97 );
        Server.Region    = Regions[ Spread( Index, 13, UE_ARRAY_COUNT( Regions ) ) ];
        Server.Mode      = Modes[ Spread( Index, 17, UE_ARRAY_COUNT( Modes ) ) ];
        Server.Map       = Maps[ Spread( Index, 29, UE_ARRAY_COUNT( Maps ) ) ];
        Server.Slots     = 16 + Spread( Index, 31, 3 ) * 8;
        Server.Players   = Spread( Index, 37, Server.Slots + 1 );
        Server.PingMs    = 12 + Spread( Index, 41, 190 );
        Server.bLocked   = Spread( Index, 43, 7 ) == 0;
        Server.bOfficial = Spread( Index, 47, 3 ) == 0;

        Servers.Add( MoveTemp( Server ) );
    }

    UE_LOGFMT( LogSmartTables, Verbose, "Server model generated {Count} server(s).", Servers.Num() );

    NotifyNumRowsChanged();
}

bool USmartTableServerModel::IsLocked( int32 NaturalRow ) const
{
    return Servers.IsValidIndex( NaturalRow ) && Servers[ NaturalRow ].bLocked;
}

FText USmartTableServerModel::GetServerName( int32 NaturalRow ) const
{
    return Servers.IsValidIndex( NaturalRow ) ? FText::FromString( Servers[ NaturalRow ].Name ) : FText::GetEmpty();
}

int32 USmartTableServerModel::GetNumRows_Implementation()
{
    return Servers.Num();
}

FText USmartTableServerModel::GetCellText_Implementation( int32 NaturalRow, FName ColumnId )
{
    if ( !Servers.IsValidIndex( NaturalRow ) )
    {
        return FText::GetEmpty();
    }

    const FServer & Server = Servers[ NaturalRow ];

    if ( ColumnId == TEXT( "Lock" ) )
    {
        return FText::GetEmpty();
    }

    if ( ColumnId == TEXT( "Name" ) )
    {
        return FText::FromString( Server.bOfficial ? FString::Printf( TEXT( "%s\nofficial · %s" ), *Server.Name, *Server.Region ) : FString::Printf( TEXT( "%s\ncommunity · %s" ), *Server.Name, *Server.Region ) );
    }

    if ( ColumnId == TEXT( "Mode" ) )
    {
        return FText::FromString( Server.Mode );
    }

    if ( ColumnId == TEXT( "Map" ) )
    {
        return FText::FromString( Server.Map );
    }

    if ( ColumnId == TEXT( "Players" ) )
    {
        return FText::Format( LOCTEXT( "ServerPlayers", "{0}/{1}" ), FText::AsNumber( Server.Players ), FText::AsNumber( Server.Slots ) );
    }

    if ( ColumnId == TEXT( "Ping" ) )
    {
        return FText::Format( LOCTEXT( "ServerPing", "{0} ms" ), FText::AsNumber( Server.PingMs ) );
    }

    return FText::GetEmpty();
}

FLinearColor USmartTableServerModel::GetCellColor_Implementation( int32 NaturalRow, FName ColumnId )
{
    if ( !Servers.IsValidIndex( NaturalRow ) )
    {
        return SmartTableDemo::NoColourOpinion();
    }

    const FServer & Server = Servers[ NaturalRow ];

    if ( ColumnId == TEXT( "Ping" ) )
    {
        if ( Server.PingMs < 60 )
        {
            return FLinearColor( 0.34f, 0.82f, 0.48f, 1.0f );
        }

        return Server.PingMs < 130 ? FLinearColor( 0.92f, 0.72f, 0.26f, 1.0f ) : FLinearColor( 0.90f, 0.34f, 0.34f, 1.0f );
    }

    if ( ColumnId == TEXT( "Lock" ) )
    {
        return Server.bLocked ? FLinearColor( 0.92f, 0.42f, 0.42f, 1.0f ) : FLinearColor( 0.30f, 0.86f, 0.72f, 1.0f );
    }

    if ( ColumnId == TEXT( "Players" ) )
    {
        const float Fill = Server.Slots > 0 ? Server.Players / static_cast< float >( Server.Slots ) : 0.0f;

        return Fill > 0.9f ? FLinearColor( 0.92f, 0.72f, 0.26f, 1.0f ) : SmartTableDemo::NoColourOpinion();
    }

    if ( ColumnId == TEXT( "Mode" ) )
    {
        return FLinearColor( 0.36f, 0.78f, 0.92f, 1.0f );
    }

    return SmartTableDemo::NoColourOpinion();
}

FSmartTableSortKey USmartTableServerModel::GetCellSortKey_Implementation( int32 NaturalRow, FName ColumnId )
{
    if ( !Servers.IsValidIndex( NaturalRow ) )
    {
        return FSmartTableSortKey::MakeEmpty();
    }

    const FServer & Server = Servers[ NaturalRow ];

    if ( ColumnId == TEXT( "Ping" ) )
    {
        return FSmartTableSortKey::MakeNumber( Server.PingMs );
    }

    if ( ColumnId == TEXT( "Players" ) )
    {
        return FSmartTableSortKey::MakeNumber( Server.Players );
    }

    if ( ColumnId == TEXT( "Lock" ) )
    {
        return FSmartTableSortKey::MakeBool( Server.bLocked );
    }

    if ( ColumnId == TEXT( "Name" ) )
    {
        return FSmartTableSortKey::MakeText( Server.Name );
    }

    if ( ColumnId == TEXT( "Mode" ) )
    {
        return FSmartTableSortKey::MakeText( Server.Mode );
    }

    if ( ColumnId == TEXT( "Map" ) )
    {
        return FSmartTableSortKey::MakeText( Server.Map );
    }

    return FSmartTableSortKey::MakeEmpty();
}

FName USmartTableServerModel::GetRowId_Implementation( int32 NaturalRow )
{
    return Servers.IsValidIndex( NaturalRow ) ? FName( TEXT( "SRV" ), NaturalRow + 1 ) : NAME_None;
}

namespace
{
    const TCHAR * ItemNames[] = { TEXT( "Cell Charge" ), TEXT( "Fuel Cell" ), TEXT( "Ration Pack" ), TEXT( "Sensor Spike" ), TEXT( "Cutting Torch" ) };

    const FLinearColor RarityColours[] = { FLinearColor( 0.52f, 0.56f, 0.60f ), FLinearColor( 0.42f, 0.78f, 0.48f ), FLinearColor( 0.36f, 0.62f, 0.92f ), FLinearColor( 0.66f, 0.46f, 0.92f ), FLinearColor( 0.95f, 0.62f, 0.28f ) };

    const TCHAR * RarityNames[] = { TEXT( "Common" ), TEXT( "Uncommon" ), TEXT( "Rare" ), TEXT( "Exotic" ), TEXT( "Relic" ) };
}
void USmartTableInventoryModel::GenerateBag( int32 InColumns, int32 InRows )
{
    Columns = FMath::Max( InColumns, 1 );
    Grid.Reset();

    for ( int32 Row = 0; Row < FMath::Max( InRows, 1 ); ++Row )
    {
        TArray< FSlot > Line;
        Line.SetNum( Columns );

        for ( int32 Column = 0; Column < Columns; ++Column )
        {
            const int32 Seed = Row * Columns + Column;

            if ( Spread( Seed, 17, 100 ) < 34 )
            {
                continue;
            }

            FSlot & Slot = Line[ Column ];
            Slot.Shape   = Spread( Seed, 3, UE_ARRAY_COUNT( ItemNames ) );
            Slot.Rarity  = Spread( Seed, 91, UE_ARRAY_COUNT( RarityColours ) );
            Slot.Name    = ItemNames[ Slot.Shape ];
            Slot.Count   = 1 + Spread( Seed, 55, 24 );
        }

        Grid.Add( MoveTemp( Line ) );
    }

    NotifyRowsChanged();
}

TArray< FName > USmartTableInventoryModel::GetSlotColumnIds() const
{
    TArray< FName > Ids;
    Ids.Reserve( Columns );

    for ( int32 Column = 0; Column < Columns; ++Column )
    {
        Ids.Add( FName( TEXT( "S" ), Column + 1 ) );
    }

    return Ids;
}

int32 USmartTableInventoryModel::IndexOfColumn( FName ColumnId ) const
{
    const int32 Number = ColumnId.GetNumber();

    return Number > 0 ? Number - 1 : INDEX_NONE;
}

const USmartTableInventoryModel::FSlot * USmartTableInventoryModel::FindSlot( int32 NaturalRow, FName ColumnId ) const
{
    const int32 Column = IndexOfColumn( ColumnId );

    return Grid.IsValidIndex( NaturalRow ) && Grid[ NaturalRow ].IsValidIndex( Column ) ? &Grid[ NaturalRow ][ Column ] : nullptr;
}

FText USmartTableInventoryModel::GetItemName( int32 NaturalRow, FName ColumnId ) const
{
    const FSlot * Slot = FindSlot( NaturalRow, ColumnId );

    return Slot && !Slot->IsEmpty() ? FText::FromString( Slot->Name ) : FText::GetEmpty();
}

FText USmartTableInventoryModel::GetItemRarity( int32 NaturalRow, FName ColumnId ) const
{
    const FSlot * Slot = FindSlot( NaturalRow, ColumnId );

    return Slot && !Slot->IsEmpty() ? FText::FromString( RarityNames[ Slot->Rarity ] ) : FText::GetEmpty();
}

void USmartTableInventoryModel::SwapSlots( int32 RowA, FName ColumnA, int32 RowB, FName ColumnB )
{
    const int32 IndexA = IndexOfColumn( ColumnA );
    const int32 IndexB = IndexOfColumn( ColumnB );

    if ( !Grid.IsValidIndex( RowA ) || !Grid.IsValidIndex( RowB ) || !Grid[ RowA ].IsValidIndex( IndexA ) || !Grid[ RowB ].IsValidIndex( IndexB ) )
    {
        return;
    }

    Swap( Grid[ RowA ][ IndexA ], Grid[ RowB ][ IndexB ] );

    NotifyCellChanged( RowA, ColumnA );
    if ( RowB != RowA || ColumnB != ColumnA )
    {
        NotifyCellChanged( RowB, ColumnB );
    }
}

int32 USmartTableInventoryModel::GetNumRows_Implementation()
{
    return Grid.Num();
}

FText USmartTableInventoryModel::GetCellText_Implementation( int32 NaturalRow, FName ColumnId )
{
    const FSlot * Slot = FindSlot( NaturalRow, ColumnId );

    return Slot && !Slot->IsEmpty() && Slot->Count > 1 ? FText::AsNumber( Slot->Count ) : FText::GetEmpty();
}

FLinearColor USmartTableInventoryModel::GetCellColor_Implementation( int32 NaturalRow, FName ColumnId )
{
    const FSlot * Slot = FindSlot( NaturalRow, ColumnId );

    return Slot && !Slot->IsEmpty() ? RarityColours[ Slot->Rarity ] : SmartTableDemo::NoColourOpinion();
}

FSmartTableSortKey USmartTableInventoryModel::GetCellSortKey_Implementation( int32 NaturalRow, FName ColumnId )
{
    const FSlot * Slot = FindSlot( NaturalRow, ColumnId );

    return Slot && !Slot->IsEmpty() ? FSmartTableSortKey::MakeNumber( Slot->Shape ) : FSmartTableSortKey::MakeEmpty();
}

FName USmartTableInventoryModel::GetRowId_Implementation( int32 NaturalRow )
{
    return Grid.IsValidIndex( NaturalRow ) ? FName( TEXT( "ROW" ), NaturalRow + 1 ) : NAME_None;
}

namespace
{
    const TCHAR * CardSymbols[] = {
        TEXT( "A" ),
        TEXT( "B" ),
        TEXT( "C" ),
        TEXT( "D" ),
        TEXT( "E" ),
        TEXT( "F" ),
        TEXT( "G" ),
        TEXT( "H" ),
        TEXT( "J" ),
        TEXT( "K" ),
        TEXT( "L" ),
        TEXT( "M" ),
        TEXT( "N" ),
        TEXT( "P" ),
        TEXT( "R" ),
        TEXT( "S" ),
    };

    const FLinearColor CardColours[] = {
        FLinearColor( 0.99f, 0.35f, 0.62f, 1.0f ),
        FLinearColor( 0.35f, 0.85f, 0.99f, 1.0f ),
        FLinearColor( 0.65f, 0.95f, 0.45f, 1.0f ),
        FLinearColor( 0.99f, 0.75f, 0.30f, 1.0f ),
        FLinearColor( 0.78f, 0.62f, 0.99f, 1.0f ),
        FLinearColor( 0.45f, 0.99f, 0.82f, 1.0f ),
        FLinearColor( 0.99f, 0.50f, 0.40f, 1.0f ),
        FLinearColor( 0.55f, 0.76f, 0.99f, 1.0f ),
        FLinearColor( 0.99f, 0.62f, 0.28f, 1.0f ),
        FLinearColor( 0.40f, 0.92f, 0.60f, 1.0f ),
        FLinearColor( 0.95f, 0.88f, 0.42f, 1.0f ),
        FLinearColor( 0.62f, 0.45f, 0.99f, 1.0f ),
        FLinearColor( 0.30f, 0.90f, 0.86f, 1.0f ),
        FLinearColor( 0.99f, 0.60f, 0.76f, 1.0f ),
        FLinearColor( 0.72f, 0.85f, 0.45f, 1.0f ),
        FLinearColor( 0.68f, 0.72f, 0.99f, 1.0f ),
    };
}
void USmartTablePairsModel::Deal( int32 InColumns, int32 InRows )
{
    Columns = FMath::Max( InColumns, 2 );

    const int32 Rows      = FMath::Max( InRows, 1 );
    const int32 Positions = Columns * Rows;

    ensureMsgf( Positions % 2 == 0, TEXT( "A pairs board needs an even number of positions; %dx%d is %d" ), Columns, Rows, Positions );

    PairsTotal = Positions / 2;

    ensureMsgf( PairsTotal <= UE_ARRAY_COUNT( CardSymbols ), TEXT( "A %dx%d board wants %d pairs but only %d symbols exist" ), Columns, Rows, PairsTotal, static_cast< int32 >( UE_ARRAY_COUNT( CardSymbols ) ) );

    TArray< int32 > Deck;
    Deck.Reserve( Positions );
    for ( int32 Pair = 0; Pair < PairsTotal; ++Pair )
    {
        const int32 Symbol = Pair % UE_ARRAY_COUNT( CardSymbols );
        Deck.Add( Symbol );
        Deck.Add( Symbol );
    }

    for ( int32 Index = Deck.Num() - 1; Index > 0; --Index )
    {
        Deck.Swap( Index, FMath::RandHelper( Index + 1 ) );
    }

    Grid.Reset();
    for ( int32 Row = 0; Row < Rows; ++Row )
    {
        TArray< FCard > Line;
        Line.SetNum( Columns );

        for ( int32 Column = 0; Column < Columns; ++Column )
        {
            Line[ Column ].Symbol = Deck[ Row * Columns + Column ];
        }

        Grid.Add( MoveTemp( Line ) );
    }

    FirstRow     = INDEX_NONE;
    SecondRow    = INDEX_NONE;
    FirstColumn  = NAME_None;
    SecondColumn = NAME_None;
    Moves        = 0;
    PairsFound   = 0;

    UE_LOGFMT( LogSmartTables, Verbose, "Pairs model dealt {Columns}x{Rows}: {Pairs} pair(s).", Columns, Rows, PairsTotal );

    NotifyRowsChanged();
}

TArray< FName > USmartTablePairsModel::GetCardColumnIds() const
{
    TArray< FName > Ids;
    Ids.Reserve( Columns );

    for ( int32 Column = 0; Column < Columns; ++Column )
    {
        Ids.Add( FName( TEXT( "C" ), Column + 1 ) );
    }

    return Ids;
}

int32 USmartTablePairsModel::IndexOfColumn( FName ColumnId ) const
{
    int32 Index = INDEX_NONE;
    return GetCardColumnIds().Find( ColumnId, Index ) ? Index : INDEX_NONE;
}

namespace
{
    template< typename TGrid >
    auto * CardAt( TGrid & Grid, int32 NaturalRow, int32 Column )
    {
        return Grid.IsValidIndex( NaturalRow ) && Grid[ NaturalRow ].IsValidIndex( Column ) ? &Grid[ NaturalRow ][ Column ] : nullptr;
    }
}
const USmartTablePairsModel::FCard * USmartTablePairsModel::FindCard( int32 NaturalRow, FName ColumnId ) const
{
    return CardAt( Grid, NaturalRow, IndexOfColumn( ColumnId ) );
}

USmartTablePairsModel::FCard * USmartTablePairsModel::FindCard( int32 NaturalRow, FName ColumnId )
{
    return CardAt( Grid, NaturalRow, IndexOfColumn( ColumnId ) );
}

void USmartTablePairsModel::FlipCard( int32 NaturalRow, FName ColumnId )
{
    FCard * Card = FindCard( NaturalRow, ColumnId );

    if ( !Card || Card->bMatched || Card->bFaceUp || SecondRow != INDEX_NONE )
    {
        OnTurn.Broadcast( ESmartTablePairsTurn::Ignored );
        return;
    }

    Card->bFaceUp = true;

    NotifyRowChanged( NaturalRow );

    if ( FirstRow == INDEX_NONE )
    {
        FirstRow    = NaturalRow;
        FirstColumn = ColumnId;

        OnTurn.Broadcast( ESmartTablePairsTurn::Revealed );
        return;
    }

    ++Moves;

    FCard * First = FindCard( FirstRow, FirstColumn );
    if ( First && First->Symbol == Card->Symbol )
    {
        First->bMatched = true;
        Card->bMatched  = true;
        ++PairsFound;

        NotifyRowChanged( FirstRow );
        NotifyRowChanged( NaturalRow );

        FirstRow    = INDEX_NONE;
        FirstColumn = NAME_None;

        OnTurn.Broadcast( ESmartTablePairsTurn::Matched );
        return;
    }

    SecondRow    = NaturalRow;
    SecondColumn = ColumnId;

    OnTurn.Broadcast( ESmartTablePairsTurn::Missed );
}

void USmartTablePairsModel::TurnBack()
{
    if ( SecondRow == INDEX_NONE )
    {
        return;
    }

    FCard * First  = FindCard( FirstRow, FirstColumn );
    FCard * Second = FindCard( SecondRow, SecondColumn );

    if ( First )
    {
        First->bFaceUp = false;
    }

    if ( Second )
    {
        Second->bFaceUp = false;
    }

    NotifyRowChanged( FirstRow );

    if ( SecondRow != FirstRow )
    {
        NotifyRowChanged( SecondRow );
    }

    FirstRow     = INDEX_NONE;
    SecondRow    = INDEX_NONE;
    FirstColumn  = NAME_None;
    SecondColumn = NAME_None;
}

bool USmartTablePairsModel::IsFaceUp( int32 NaturalRow, FName ColumnId ) const
{
    const FCard * Card = FindCard( NaturalRow, ColumnId );

    return Card && ( Card->bFaceUp || Card->bMatched );
}

bool USmartTablePairsModel::IsMatched( int32 NaturalRow, FName ColumnId ) const
{
    const FCard * Card = FindCard( NaturalRow, ColumnId );

    return Card && Card->bMatched;
}

int32 USmartTablePairsModel::GetSymbol( int32 NaturalRow, FName ColumnId ) const
{
    const FCard * Card = FindCard( NaturalRow, ColumnId );

    return Card ? Card->Symbol : INDEX_NONE;
}

int32 USmartTablePairsModel::GetNumRows_Implementation()
{
    return Grid.Num();
}

FText USmartTablePairsModel::GetCellText_Implementation( int32 NaturalRow, FName ColumnId )
{
    const FCard * Card = FindCard( NaturalRow, ColumnId );

    if ( !Card || !( Card->bFaceUp || Card->bMatched ) )
    {
        return FText::GetEmpty();
    }

    return FText::FromString( CardSymbols[ Card->Symbol % UE_ARRAY_COUNT( CardSymbols ) ] );
}

FLinearColor USmartTablePairsModel::GetCellColor_Implementation( int32 NaturalRow, FName ColumnId )
{
    const FCard * Card = FindCard( NaturalRow, ColumnId );
    if ( !Card || !( Card->bFaceUp || Card->bMatched ) )
    {
        return FLinearColor::Transparent;
    }

    return CardColours[ Card->Symbol % UE_ARRAY_COUNT( CardColours ) ];
}

FName USmartTablePairsModel::GetRowId_Implementation( int32 NaturalRow )
{
    return Grid.IsValidIndex( NaturalRow ) ? FName( TEXT( "CARDROW" ), NaturalRow + 1 ) : NAME_None;
}

#undef LOCTEXT_NAMESPACE
