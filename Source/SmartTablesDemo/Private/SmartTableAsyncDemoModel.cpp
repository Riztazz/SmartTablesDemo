#include "SmartTableAsyncDemoModel.h"
#include "SmartTableDemoConstants.h"

#include "HAL/PlatformProcess.h"
#include "Logging/StructuredLog.h"
#include "SmartTableDispatcher.h"
#include "SmartTableLog.h"
#include "Tasks/Task.h"

#define LOCTEXT_NAMESPACE "SmartTablesDemo"

namespace
{
    const TCHAR * Stems[] = { TEXT( "Vanta" ), TEXT( "Garris" ), TEXT( "Keller" ), TEXT( "Sable" ), TEXT( "Doran" ), TEXT( "Cinder" ), TEXT( "Halcyon" ), TEXT( "Mott" ), TEXT( "Ridge" ), TEXT( "Tarn" ) };

    const TCHAR * Kinds[] = { TEXT( "Asteroid" ), TEXT( "Satellite" ), TEXT( "Derelict" ), TEXT( "Booster" ), TEXT( "Station" ), TEXT( "Probe" ) };

    const TCHAR * Owners[] = { TEXT( "UNSA" ), TEXT( "Kepler Ltd" ), TEXT( "Unknown" ), TEXT( "Reclaimed" ), TEXT( "Orbital Guild" ) };

    const TCHAR * Notes[] = {
        TEXT( "Tumbling slowly on two axes; the docking window is short and the approach has to be flown by hand." ),
        TEXT( "Hull intact. Reports a faint carrier signal on the old survey band, source unconfirmed." ),
        TEXT( "Stripped years ago. Whatever is left is welded down and not worth the fuel to reach it." ),
        TEXT( "No response to hail." ),
    };

    const TCHAR * Summaries[] = {
        TEXT( "Surveyed from a distance during the second sweep. The spectrum is mostly nickel-iron with a silicate crust, and the return is good enough that a second visit was pencilled in and never made. Approach on the leading side; the trailing face is still shedding." ),
        TEXT( "Registered to a company that no longer files returns. The transponder answers, the crew manifest does not, and the last docking log is eleven years old. Whatever is aboard has been aboard a long time." ),
        TEXT( "Written off after the coolant loop failed. The hull is sound and the racks are intact, which makes it the best salvage on this inclination if you can carry the mass home." ),
        TEXT( "Nothing on any band. No debris field, no thermal signature, no reason to stop." ),
    };

    template< typename T, int32 N >
    const TCHAR * Pick( T ( &Table )[ N ], int32 Index )
    {
        return Table[ Index % N ];
    }
}
void USmartTableAsyncDemoModel::GenerateRows( int32 NumRows )
{
    Rows.Reset();
    Rows.SetNumUninitialized( FMath::Max( NumRows, 0 ) );

    for ( int32 Index = 0; Index < Rows.Num(); ++Index )
    {
        FRow & Row = Rows[ Index ];

        Row.DistanceKm   = 4.0 + FMath::Fmod( Index * 7.31, 4200.0 );
        Row.DeltaV       = 12.0 + FMath::Fmod( Index * 3.7, 900.0 );
        Row.MassTonnes   = 120.0 + FMath::Fmod( Index * 53.0, 90000.0 );
        Row.Heading      = static_cast< float >( ( Index * 23 ) % 360 );
        Row.Integrity    = static_cast< float >( ( Index * 17 ) % 101 );
        Row.CrewCapacity = ( Index * 3 ) % 12;
        Row.bVisited     = ( Index % 5 ) == 0;
        Row.Status       = static_cast< ESmartTableDemoStatus >( Index % 5 );
    }

    UE_LOGFMT( LogSmartTables, Verbose, "Async demo model generated {Rows} row(s) of plain structs - no UObject per row.", Rows.Num() );

    NotifyNumRowsChanged();
}

void USmartTableAsyncDemoModel::SetCellValue( int32 NaturalRow, FName ColumnId, float Value )
{
    if ( !Rows.IsValidIndex( NaturalRow ) )
    {
        UE_LOGFMT( LogSmartTables, Verbose, "Edit dropped: row {Row} is past the end of {Count} row(s).", NaturalRow, Rows.Num() );
        return;
    }

    FRow & Row = Rows[ NaturalRow ];

    if ( ColumnId == TEXT( "Heading" ) )
    {
        Row.Heading = Value;
    }
    else if ( ColumnId == TEXT( "Integrity" ) )
    {
        Row.Integrity = Value;
    }
    else
    {
        UE_LOGFMT( LogSmartTables, Verbose, "Edit dropped: this model has nowhere to store '{Column}'.", ColumnId );
        return;
    }

    NotifyRowChanged( NaturalRow );
}

void USmartTableAsyncDemoModel::UseAsyncSorting( float SimulatedLatencySeconds )
{
    UE_LOGFMT( LogSmartTables, Verbose, "Async demo model will sort off the game thread with {Latency} s of pretend latency.", SimulatedLatencySeconds );

    SetWorkDispatcher( [ SimulatedLatencySeconds ]( TUniqueFunction< void() > Work )
    {
        UE::Tasks::Launch( UE_SOURCE_LOCATION, [ SimulatedLatencySeconds, Work = MoveTemp( Work ) ]() mutable
        {
            if ( SimulatedLatencySeconds > 0.0f )
            {
                FPlatformProcess::Sleep( SimulatedLatencySeconds );
            }

            Work();
        } );
    } );
}

void USmartTableAsyncDemoModel::UseStatusRowColors( bool bEnabled )
{
    bStatusRowColors = bEnabled;

    NotifyRowsChanged();
}

FLinearColor USmartTableAsyncDemoModel::GetRowColor_Implementation( int32 NaturalRow )
{
    if ( !bStatusRowColors || !Rows.IsValidIndex( NaturalRow ) )
    {
        return SmartTableDemo::NoColourOpinion();
    }

    switch ( Rows[ NaturalRow ].Status )
    {
        case ESmartTableDemoStatus::Claimed:
            return FLinearColor( 0.72f, 0.94f, 0.78f, 1.0f );

        case ESmartTableDemoStatus::Derelict:
            return FLinearColor( 0.98f, 0.80f, 0.66f, 1.0f );

        case ESmartTableDemoStatus::Restricted:
            return FLinearColor( 0.98f, 0.70f, 0.72f, 1.0f );

        case ESmartTableDemoStatus::Surveyed:
            return FLinearColor( 0.76f, 0.86f, 0.99f, 1.0f );

        default:
            return SmartTableDemo::NoColourOpinion();
    }
}

void USmartTableAsyncDemoModel::UseSynchronousSorting()
{
    UE_LOGFMT( LogSmartTables, Verbose, "Async demo model back to sorting inline on the game thread." );

    SetWorkDispatcher( FSmartTableWorkDispatcher() );
}

int32 USmartTableAsyncDemoModel::GetNumRows_Implementation()
{
    return Rows.Num();
}

FText USmartTableAsyncDemoModel::GetCellText_Implementation( int32 NaturalRow, FName ColumnId )
{
    if ( !Rows.IsValidIndex( NaturalRow ) )
    {
        return FText::GetEmpty();
    }

    const FRow & Row = Rows[ NaturalRow ];

    if ( ColumnId == TEXT( "Callsign" ) )
    {
        return FText::FromString( FString::Printf( TEXT( "%s-%d" ), Pick( Stems, NaturalRow ), NaturalRow + 1 ) );
    }

    if ( ColumnId == TEXT( "Designation" ) )
    {
        return FText::FromString( FString::Printf( TEXT( "OBJ-%06d" ), NaturalRow + 1 ) );
    }

    if ( ColumnId == TEXT( "Class" ) )
    {
        return FText::FromString( Pick( Kinds, NaturalRow ) );
    }

    if ( ColumnId == TEXT( "Owner" ) )
    {
        return FText::FromString( Pick( Owners, NaturalRow ) );
    }

    if ( ColumnId == TEXT( "Notes" ) )
    {
        return FText::FromString( Pick( Notes, NaturalRow ) );
    }

    if ( ColumnId == TEXT( "Summary" ) )
    {
        return FText::FromString( Pick( Summaries, NaturalRow ) );
    }

    if ( ColumnId == TEXT( "DistanceKm" ) )
    {
        return FText::AsNumber( Row.DistanceKm );
    }

    if ( ColumnId == TEXT( "DeltaV" ) )
    {
        return FText::AsNumber( Row.DeltaV );
    }

    if ( ColumnId == TEXT( "MassTonnes" ) )
    {
        return FText::AsNumber( Row.MassTonnes );
    }

    if ( ColumnId == TEXT( "Integrity" ) )
    {
        return FText::AsNumber( Row.Integrity );
    }

    if ( ColumnId == TEXT( "Heading" ) )
    {
        return FText::AsNumber( Row.Heading );
    }

    if ( ColumnId == TEXT( "CrewCapacity" ) )
    {
        return FText::AsNumber( Row.CrewCapacity );
    }

    return FText::GetEmpty();
}

FSmartTableSortKey USmartTableAsyncDemoModel::GetCellSortKey_Implementation( int32 NaturalRow, FName ColumnId )
{
    if ( !Rows.IsValidIndex( NaturalRow ) )
    {
        return FSmartTableSortKey::MakeEmpty();
    }

    const FRow & Row = Rows[ NaturalRow ];

    if ( ColumnId == TEXT( "DistanceKm" ) )
    {
        return FSmartTableSortKey::MakeNumber( Row.DistanceKm );
    }

    if ( ColumnId == TEXT( "MassTonnes" ) )
    {
        return FSmartTableSortKey::MakeNumber( Row.MassTonnes );
    }

    if ( ColumnId == TEXT( "Integrity" ) )
    {
        return FSmartTableSortKey::MakeNumber( Row.Integrity );
    }

    if ( ColumnId == TEXT( "Heading" ) )
    {
        return FSmartTableSortKey::MakeNumber( Row.Heading );
    }

    if ( ColumnId == TEXT( "DeltaV" ) )
    {
        return FSmartTableSortKey::MakeNumber( Row.DeltaV );
    }

    if ( ColumnId == TEXT( "CrewCapacity" ) )
    {
        return FSmartTableSortKey::MakeNumber( Row.CrewCapacity );
    }

    return Super::GetCellSortKey_Implementation( NaturalRow, ColumnId );
}

FName USmartTableAsyncDemoModel::GetRowId_Implementation( int32 NaturalRow )
{
    return Rows.IsValidIndex( NaturalRow ) ? FName( TEXT( "OBJ" ), NaturalRow + 1 ) : NAME_None;
}

#undef LOCTEXT_NAMESPACE
