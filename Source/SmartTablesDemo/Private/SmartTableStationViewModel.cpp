#include "SmartTableStationViewModel.h"

#include "Internationalization/Text.h"
#include "Math/UnrealMathUtility.h"

#define LOCTEXT_NAMESPACE "SmartTablesDemo"

namespace
{

    constexpr double LowestStationPressureHpa  = 950.0;
    constexpr double HighestStationPressureHpa = 1060.0;

    FText StationPressureText( double PressureHpa )
    {
        return FText::Format( LOCTEXT( "StationPressure", "{0} hPa" ), FText::AsNumber( FMath::RoundToInt( PressureHpa ) ) );
    }

    float StationPressureFraction( double PressureHpa )
    {
        return static_cast< float >( FMath::GetRangePct( LowestStationPressureHpa, HighestStationPressureHpa, PressureHpa ) );
    }

    FText StationHumidityText( int32 Humidity )
    {
        return FText::Format( LOCTEXT( "StationHumidity", "{0}%" ), FText::AsNumber( Humidity ) );
    }
}

USmartTableStationViewModel::USmartTableStationViewModel()
    : PressureText( StationPressureText( PressureHpa ) )
    , PressureFraction( 0.0f )
    , HumidityText( StationHumidityText( Humidity ) )
{
}

void USmartTableStationViewModel::SetPressureHpa( double NewPressureHpa )
{
    if ( UE_MVVM_SET_PROPERTY_VALUE( PressureHpa, FMath::Clamp( NewPressureHpa, LowestStationPressureHpa, HighestStationPressureHpa ) ) )
    {
        UE_MVVM_SET_PROPERTY_VALUE( PressureText, StationPressureText( PressureHpa ) );
        UE_MVVM_SET_PROPERTY_VALUE( PressureFraction, StationPressureFraction( PressureHpa ) );
    }
}

void USmartTableStationViewModel::SetHumidity( int32 NewHumidity )
{
    if ( UE_MVVM_SET_PROPERTY_VALUE( Humidity, FMath::Clamp( NewHumidity, 0, 100 ) ) )
    {
        UE_MVVM_SET_PROPERTY_VALUE( HumidityText, StationHumidityText( Humidity ) );
        UE_MVVM_SET_PROPERTY_VALUE( HumidityFraction, Humidity / 100.0f );
    }
}

#undef LOCTEXT_NAMESPACE
