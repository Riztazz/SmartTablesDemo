#pragma once

#include "Containers/UnrealString.h"
#include "Layout/Margin.h"
#include "Math/Color.h"

namespace SmartTableDemo
{
    inline FString SharedAsset( const TCHAR * AssetName )
    {
        return FString( TEXT( "/Game/Demo/Shared/" ) ) + AssetName;
    }

    inline FLinearColor NoColourOpinion()
    {
        return FLinearColor( 0.0f, 0.0f, 0.0f, 0.0f );
    }

    inline FMargin PositionCellPadding()
    {
        return FMargin( 2.0f );
    }
}
