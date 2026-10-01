#include "SmartTableShowcaseCells.h"
#include "SmartTableCellReads.h"

#include "Animation/WidgetAnimation.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Logging/StructuredLog.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Rendering/DrawElements.h"
#include "SmartTable.h"
#include "SmartTableLog.h"
#include "SmartTableModel.h"
#include "SmartTableShowcaseModels.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SmartTables"

namespace
{
    FText ReadText( const USmartTableCell & Cell )
    {
        USmartTableModel * Model = Cell.GetModel();

        return Model ? Model->GetCellText( Cell.GetRowIndex(), Cell.GetColumnId() ) : FText::GetEmpty();
    }

    FLinearColor ReadAccent( const USmartTableCell & Cell )
    {
        USmartTableModel * Model        = Cell.GetModel();
        const USmartTable * OwningTable = Cell.GetTable();

        const FLinearColor Fallback = OwningTable ? OwningTable->GetCellTextStyle().ColorAndOpacity.GetSpecifiedColor() : FLinearColor::White;

        if ( !Model )
        {
            return Fallback;
        }

        const FLinearColor Accent = Model->GetCellColor( Cell.GetRowIndex(), Cell.GetColumnId() );

        return Accent.A > 0.0f ? Accent : Fallback;
    }

    FSlateResourceHandle WhiteHandle()
    {
        const FSlateBrush * White = FCoreStyle::Get().GetBrush( "WhiteBrush" );

        return FSlateApplication::Get().GetRenderer()->GetResourceHandle( *White );
    }

    FSlateVertex Vert( const FGeometry & Geometry, float X, float Y, const FColor & Colour )
    {
        FSlateVertex V;
        V.Position       = FVector2f( Geometry.LocalToAbsolute( FVector2f( X, Y ) ) );
        V.Color          = Colour;
        V.TexCoords[ 0 ] = 0.5f;
        V.TexCoords[ 1 ] = 0.5f;
        V.TexCoords[ 2 ] = 0.5f;
        V.TexCoords[ 3 ] = 0.5f;

        return V;
    }

    void AddRect( TArray< FSlateVertex > & Verts, TArray< SlateIndex > & Indices, const FGeometry & Geometry, float Left, float Top, float Right, float Bottom, const FColor & Colour )
    {
        if ( Right <= Left || Bottom <= Top )
        {
            return;
        }

        const SlateIndex Base = static_cast< SlateIndex >( Verts.Num() );

        Verts.Add( Vert( Geometry, Left, Top, Colour ) );
        Verts.Add( Vert( Geometry, Right, Top, Colour ) );
        Verts.Add( Vert( Geometry, Right, Bottom, Colour ) );
        Verts.Add( Vert( Geometry, Left, Bottom, Colour ) );

        Indices.Append( { Base, SlateIndex( Base + 1 ), SlateIndex( Base + 2 ), Base, SlateIndex( Base + 2 ), SlateIndex( Base + 3 ) } );
    }
}

class SSmartTableChip : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS( SSmartTableChip )
    {
    }
    SLATE_END_ARGS()

    void Construct( const FArguments & )
    {
    }

    void SetContent( const FText & InLabel, const FLinearColor & InAccent )
    {
        Label  = InLabel;
        Accent = InAccent;
    }

    void SetTextStyle( const FTextBlockStyle & InStyle )
    {
        Font = InStyle.Font;
    }

    virtual FVector2D ComputeDesiredSize( float ) const override
    {
        const TSharedRef< FSlateFontMeasure > Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        const FVector2D Text                          = Measure->Measure( Label, Font );

        return FVector2D( Text.X + 22.0f, FMath::Max( Text.Y + 6.0f, 18.0f ) );
    }

    virtual int32 OnPaint( const FPaintArgs & Args, const FGeometry & AllottedGeometry, const FSlateRect & CullingRect, FSlateWindowElementList & OutDrawElements, int32 LayerId, const FWidgetStyle & InWidgetStyle, bool bParentEnabled ) const override
    {
        const FVector2f Local = AllottedGeometry.GetLocalSize();
        if ( Local.X <= 2.0f || Local.Y <= 2.0f )
        {
            return LayerId;
        }

        const float Height  = FMath::Min( Local.Y, 20.0f );
        const float Top     = ( Local.Y - Height ) * 0.5f;
        const float Bottom  = Top + Height;
        const float Chamfer = Height * 0.28f;

        const FColor Fill = ( Accent * FLinearColor( 1.0f, 1.0f, 1.0f, 0.16f ) ).ToFColor( false );
        const FColor Rim  = Accent.ToFColor( false );

        TArray< FSlateVertex > Verts;
        TArray< SlateIndex > Indices;

        AddRect( Verts, Indices, AllottedGeometry, Chamfer, Top, Local.X - Chamfer, Bottom, Fill );
        AddRect( Verts, Indices, AllottedGeometry, 0.0f, Top + Chamfer, Chamfer, Bottom - Chamfer, Fill );
        AddRect( Verts, Indices, AllottedGeometry, Local.X - Chamfer, Top + Chamfer, Local.X, Bottom - Chamfer, Fill );

        AddRect( Verts, Indices, AllottedGeometry, Chamfer, Top, Local.X - Chamfer, Top + 1.0f, Rim );
        AddRect( Verts, Indices, AllottedGeometry, Chamfer, Bottom - 1.0f, Local.X - Chamfer, Bottom, Rim );

        if ( Indices.Num() > 0 )
        {
            FSlateDrawElement::MakeCustomVerts( OutDrawElements, LayerId, WhiteHandle(), Verts, Indices, nullptr, 0, 0 );
        }

        const TSharedRef< FSlateFontMeasure > Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
        const FVector2D TextSize                      = Measure->Measure( Label, Font );

        FSlateDrawElement::MakeText( OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry( FVector2f( Local.X, Local.Y ), FSlateLayoutTransform( FVector2f( ( Local.X - TextSize.X ) * 0.5f, ( Local.Y - TextSize.Y ) * 0.5f ) ) ), Label, Font, ESlateDrawEffect::None, Accent );

        return LayerId + 2;
    }

private:
    FText Label;
    FLinearColor Accent = FLinearColor::White;
    FSlateFontInfo Font = FCoreStyle::GetDefaultFontStyle( "Regular", 9 );
};

TSharedRef< SWidget > USmartTableChipCell::RebuildWidget()
{
    return SNew( SBox ).VAlign( VAlign_Center ).HAlign( HAlign_Left )[ SAssignNew( Chip, SSmartTableChip ) ];
}

void USmartTableChipCell::NativeOnCellAssigned()
{
    if ( !Chip.IsValid() )
    {
        UE_LOGFMT( LogSmartTables, VeryVerbose, "Chip cell for '{Column}' has no chip widget yet.", GetColumnId() );
        return;
    }

    if ( const USmartTable * OwningTable = GetTable() )
    {
        Chip->SetTextStyle( OwningTable->GetCellTextStyle() );
    }

    Chip->SetContent( ReadText( *this ), ReadAccent( *this ) );
}

class SSmartTableSignal : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS( SSmartTableSignal )
    {
    }
    SLATE_END_ARGS()

    void Construct( const FArguments & )
    {
    }

    void SetStrength( TOptional< float > InStrength, const FLinearColor & InAccent )
    {
        Strength = InStrength;
        Accent   = InAccent;
    }

    virtual FVector2D ComputeDesiredSize( float ) const override
    {
        return FVector2D( 34.0f, 16.0f );
    }

    virtual int32 OnPaint( const FPaintArgs & Args, const FGeometry & AllottedGeometry, const FSlateRect & CullingRect, FSlateWindowElementList & OutDrawElements, int32 LayerId, const FWidgetStyle & InWidgetStyle, bool bParentEnabled ) const override
    {
        const FVector2f Local = AllottedGeometry.GetLocalSize();
        constexpr int32 Bars  = 5;

        const float Gap      = 2.0f;
        const float BarWidth = FMath::Max( ( Local.X - Gap * ( Bars - 1 ) ) / Bars, 1.0f );
        const float Lit      = Strength.IsSet() ? FMath::Clamp( Strength.GetValue(), 0.0f, 1.0f ) * Bars : 0.0f;

        const FColor On  = Accent.ToFColor( false );
        const FColor Off = ( Accent * FLinearColor( 1.0f, 1.0f, 1.0f, 0.16f ) ).ToFColor( false );

        TArray< FSlateVertex > Verts;
        TArray< SlateIndex > Indices;

        for ( int32 Bar = 0; Bar < Bars; ++Bar )
        {
            const float Height = Local.Y * ( 0.34f + 0.66f * ( Bar / static_cast< float >( Bars - 1 ) ) );
            const float Left   = Bar * ( BarWidth + Gap );

            AddRect( Verts, Indices, AllottedGeometry, Left, Local.Y - Height, Left + BarWidth, Local.Y, Bar < FMath::RoundToInt( Lit ) ? On : Off );
        }

        if ( Indices.Num() > 0 )
        {
            FSlateDrawElement::MakeCustomVerts( OutDrawElements, LayerId, WhiteHandle(), Verts, Indices, nullptr, 0, 0 );
        }

        return LayerId + 1;
    }

private:
    TOptional< float > Strength;
    FLinearColor Accent = FLinearColor::White;
};

TSharedRef< SWidget > USmartTableSignalCell::RebuildWidget()
{
    return SNew( SBox ).VAlign( VAlign_Center ).HAlign( HAlign_Left ).WidthOverride( 34.0f ).HeightOverride( 16.0f )[ SAssignNew( Signal, SSmartTableSignal ) ];
}

void USmartTableSignalCell::NativeOnCellAssigned()
{
    if ( !Signal.IsValid() )
    {
        UE_LOGFMT( LogSmartTables, VeryVerbose, "Signal cell for '{Column}' has no bars widget yet.", GetColumnId() );
        return;
    }

    const TOptional< double > Ping    = SmartTableDemo::ReadNumber( *this );
    const TOptional< float > Strength = Ping.IsSet() ? TOptional< float >( 1.0f - FMath::Clamp( ( static_cast< float >( Ping.GetValue() ) - 20.0f ) / 180.0f, 0.0f, 1.0f ) ) : TOptional< float >();

    Signal->SetStrength( Strength, ReadAccent( *this ) );
}

class SSmartTablePip : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS( SSmartTablePip )
    {
    }
    SLATE_END_ARGS()

    void Construct( const FArguments & )
    {
    }

    void SetAccent( const FLinearColor & InAccent )
    {
        Accent = InAccent;
    }

    virtual FVector2D ComputeDesiredSize( float ) const override
    {
        return FVector2D( 14.0f, 14.0f );
    }

    virtual int32 OnPaint( const FPaintArgs & Args, const FGeometry & AllottedGeometry, const FSlateRect & CullingRect, FSlateWindowElementList & OutDrawElements, int32 LayerId, const FWidgetStyle & InWidgetStyle, bool bParentEnabled ) const override
    {
        const FVector2f Local  = AllottedGeometry.GetLocalSize();
        const FVector2f Centre = Local * 0.5f;
        const float Radius     = FMath::Min( Local.X, Local.Y ) * 0.5f;
        if ( Radius <= 1.0f )
        {
            return LayerId;
        }

        TArray< FSlateVertex > Verts;
        TArray< SlateIndex > Indices;

        auto AddDisc = [ & ]( float DiscRadius, const FColor & Colour )
        {
            constexpr int32 Steps    = 24;
            const SlateIndex Centre0 = static_cast< SlateIndex >( Verts.Num() );

            Verts.Add( Vert( AllottedGeometry, Centre.X, Centre.Y, Colour ) );

            for ( int32 Step = 0; Step < Steps; ++Step )
            {
                const float Turn = ( Step / static_cast< float >( Steps ) ) * 2.0f * PI;
                Verts.Add( Vert( AllottedGeometry, Centre.X + FMath::Sin( Turn ) * DiscRadius, Centre.Y - FMath::Cos( Turn ) * DiscRadius, Colour ) );
            }

            for ( int32 Step = 0; Step < Steps; ++Step )
            {
                Indices.Add( Centre0 );
                Indices.Add( static_cast< SlateIndex >( Centre0 + 1 + Step ) );
                Indices.Add( static_cast< SlateIndex >( Centre0 + 1 + ( Step + 1 ) % Steps ) );
            }
        };

        AddDisc( Radius, ( Accent * FLinearColor( 1.0f, 1.0f, 1.0f, 0.22f ) ).ToFColor( false ) );
        AddDisc( Radius * 0.45f, Accent.ToFColor( false ) );

        FSlateDrawElement::MakeCustomVerts( OutDrawElements, LayerId, WhiteHandle(), Verts, Indices, nullptr, 0, 0 );

        return LayerId + 1;
    }

private:
    FLinearColor Accent = FLinearColor::White;
};

TSharedRef< SWidget > USmartTablePipCell::RebuildWidget()
{
    return SNew( SBox ).VAlign( VAlign_Center ).HAlign( HAlign_Center ).WidthOverride( 14.0f ).HeightOverride( 14.0f )[ SAssignNew( Pip, SSmartTablePip ) ];
}

void USmartTablePipCell::NativeOnCellAssigned()
{
    if ( !Pip.IsValid() )
    {
        UE_LOGFMT( LogSmartTables, VeryVerbose, "Pip cell for '{Column}' has no pip widget yet.", GetColumnId() );
        return;
    }

    Pip->SetAccent( ReadAccent( *this ) );
}

TSharedRef< SWidget > USmartTableStackedCell::RebuildWidget()
{

    return SNew( SVerticalBox )
        + SVerticalBox::Slot()
            .AutoHeight()
            [
                SAssignNew( TitleText, STextBlock )
                    .Clipping( EWidgetClipping::ClipToBounds )
            ]
        + SVerticalBox::Slot()
            .AutoHeight()
            [
                SAssignNew( SubtitleText, STextBlock )
                    .Clipping( EWidgetClipping::ClipToBounds )
            ];

}

void USmartTableStackedCell::NativeOnCellAssigned()
{
    if ( !TitleText.IsValid() || !SubtitleText.IsValid() )
    {
        UE_LOGFMT( LogSmartTables, VeryVerbose, "Stacked cell for '{Column}' has no text blocks yet.", GetColumnId() );
        return;
    }

    if ( const USmartTable * OwningTable = GetTable() )
    {
        TitleText->SetTextStyle( &OwningTable->GetCellTextStyle(), true );
        SubtitleText->SetTextStyle( &OwningTable->GetCellTextStyle(), true );
    }

    FString Whole = ReadText( *this ).ToString();
    FString Title;
    FString Subtitle;
    if ( !Whole.Split( TEXT( "\n" ), &Title, &Subtitle ) )
    {
        Title = MoveTemp( Whole );
    }

    TitleText->SetText( FText::FromString( Title ) );
    SubtitleText->SetText( FText::FromString( Subtitle ) );

    const FLinearColor Accent = ReadAccent( *this );
    TitleText->SetColorAndOpacity( Accent );

    SubtitleText->SetColorAndOpacity( Accent * FLinearColor( 1.0f, 1.0f, 1.0f, 0.45f ) );
    SubtitleText->SetVisibility( Subtitle.IsEmpty() ? EVisibility::Collapsed : EVisibility::HitTestInvisible );
}

class SSmartTableSlot : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS( SSmartTableSlot )
    {
    }
    SLATE_END_ARGS()

    void Construct( const FArguments & )
    {
    }

    void SetSlot( TOptional< int32 > InShape, const FLinearColor & InAccent )
    {
        Shape  = InShape;
        Accent = InAccent;
    }

    virtual FVector2D ComputeDesiredSize( float ) const override
    {
        return FVector2D( 88.0f, 84.0f );
    }

    virtual int32 OnPaint( const FPaintArgs & Args, const FGeometry & AllottedGeometry, const FSlateRect & CullingRect, FSlateWindowElementList & OutDrawElements, int32 LayerId, const FWidgetStyle & InWidgetStyle, bool bParentEnabled ) const override
    {
        const FVector2f Local = AllottedGeometry.GetLocalSize();
        const float Inset     = 3.0f;
        const float L         = Inset;
        const float T         = Inset;
        const float R         = Local.X - Inset;
        const float B         = Local.Y - Inset;

        TArray< FSlateVertex > Verts;
        TArray< SlateIndex > Indices;

        const FColor Well = ( Accent * FLinearColor( 1.0f, 1.0f, 1.0f, Shape.IsSet() ? 0.16f : 0.07f ) ).ToFColor( false );
        AddRect( Verts, Indices, AllottedGeometry, L, T, R, B, Well );

        if ( Shape.IsSet() )
        {
            const FColor Edge = Accent.ToFColor( false );
            AddRect( Verts, Indices, AllottedGeometry, L, T, R, T + 1.5f, Edge );
            AddRect( Verts, Indices, AllottedGeometry, L, B - 1.5f, R, B, Edge );
            AddRect( Verts, Indices, AllottedGeometry, L, T, L + 1.5f, B, Edge );
            AddRect( Verts, Indices, AllottedGeometry, R - 1.5f, T, R, B, Edge );

            AddGlyph( Verts, Indices, AllottedGeometry, L, T, R, B, Edge );
        }

        if ( Indices.Num() > 0 )
        {
            FSlateDrawElement::MakeCustomVerts( OutDrawElements, LayerId, WhiteHandle(), Verts, Indices, nullptr, 0, 0 );
        }

        return LayerId + 1;
    }

private:
    void AddGlyph( TArray< FSlateVertex > & Verts, TArray< SlateIndex > & Indices, const FGeometry & Geometry, float L, float T, float R, float B, const FColor & Colour ) const
    {
        const float CX   = ( L + R ) * 0.5f;
        const float CY   = ( T + B ) * 0.5f;
        const float Size = FMath::Min( R - L, B - T ) * 0.42f;

        switch ( Shape.GetValue() % 5 )
        {
            case 0:
                AddRect( Verts, Indices, Geometry, CX - Size * 0.35f, CY - Size, CX + Size * 0.35f, CY + Size, Colour );
                break;

            case 1:
                for ( int32 Step = 0; Step < 6; ++Step )
                {
                    const float Frac = Step / 6.0f;
                    const float Half = Size * ( 1.0f - Frac );
                    AddRect( Verts, Indices, Geometry, CX - Half, CY - Size + Frac * 2.0f * Size, CX + Half, CY - Size + ( Frac + 0.17f ) * 2.0f * Size, Colour );
                }
                break;

            case 2:
                AddRect( Verts, Indices, Geometry, CX - Size, CY - Size, CX + Size, CY + Size, Colour );
                break;

            case 3:
                for ( int32 Step = 0; Step < 4; ++Step )
                {
                    const float Offset = Size * ( Step / 4.0f );
                    AddRect( Verts, Indices, Geometry, CX - Size + Offset, CY - Size * 0.6f + Offset, CX - Size + Offset + Size * 0.45f, CY - Size * 0.15f + Offset, Colour );
                }
                break;

            default:
                for ( int32 Step = 0; Step < 7; ++Step )
                {
                    const float Frac = FMath::Abs( Step - 3.0f ) / 3.0f;
                    const float Half = Size * ( 1.0f - Frac );
                    AddRect( Verts, Indices, Geometry, CX - Half, CY - Size + Step * ( 2.0f * Size / 7.0f ), CX + Half, CY - Size + ( Step + 1 ) * ( 2.0f * Size / 7.0f ), Colour );
                }
                break;
        }
    }

    TOptional< int32 > Shape;
    FLinearColor Accent = FLinearColor::White;
};

TSharedRef< SWidget > USmartTableSlotCell::RebuildWidget()
{

    return SNew( SOverlay )
        + SOverlay::Slot()
            [
                SAssignNew( Slot, SSmartTableSlot )
            ]
        + SOverlay::Slot()
            .HAlign( HAlign_Right )
            .VAlign( VAlign_Bottom )
            .Padding( FMargin( 0.0f, 0.0f, 6.0f, 3.0f ) )
            [
                SAssignNew( CountText, STextBlock )
                    .Font( FCoreStyle::GetDefaultFontStyle( TEXT( "Bold" ), 11 ) )
                    .ColorAndOpacity( FSlateColor( FLinearColor::White ) )
            ];

}

void USmartTableSlotCell::NativeOnCellAssigned()
{
    if ( !Slot.IsValid() )
    {
        UE_LOGFMT( LogSmartTables, VeryVerbose, "Slot cell for '{Column}' has no slot widget yet.", GetColumnId() );
        return;
    }

    const TOptional< double > ShapeIndex = SmartTableDemo::ReadNumber( *this );
    const TOptional< int32 > Shape       = ShapeIndex.IsSet() ? TOptional< int32 >( static_cast< int32 >( ShapeIndex.GetValue() ) ) : TOptional< int32 >();

    Slot->SetSlot( Shape, ReadAccent( *this ) );

    if ( CountText.IsValid() )
    {
        CountText->SetText( ReadText( *this ) );
    }
}

void USmartTableCardCell::Flip()
{
    if ( USmartTablePairsModel * Board = Cast< USmartTablePairsModel >( GetModel() ) )
    {
        Board->FlipCard( GetRowIndex(), GetColumnId() );
    }
}

FReply USmartTableCardCell::NativeOnMouseButtonDown( const FGeometry & Geometry, const FPointerEvent & MouseEvent )
{
    if ( MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton )
    {
        return FReply::Unhandled();
    }

    Flip();

    return FReply::Handled();
}

void USmartTableCardCell::NativeOnCellAssigned()
{
    USmartTablePairsModel * Board = Cast< USmartTablePairsModel >( GetModel() );

    if ( !Board )
    {
        return;
    }

    const bool bFaceUp  = Board->IsFaceUp( GetRowIndex(), GetColumnId() );
    const bool bMatched = Board->IsMatched( GetRowIndex(), GetColumnId() );

    if ( SymbolText )
    {
        SymbolText->SetText( Board->GetCellText( GetRowIndex(), GetColumnId() ) );
        SymbolText->SetColorAndOpacity( FSlateColor( Board->GetCellColor( GetRowIndex(), GetColumnId() ) ) );
    }

    if ( Face )
    {
        Face->SetVisibility( bFaceUp ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed );

        if ( !bMatched )
        {
            if ( UMaterialInstanceDynamic * Surface = Face->GetDynamicMaterial() )
            {
                Surface->SetScalarParameterValue( DissolveParameter, 0.0f );
            }
        }
    }

    if ( Back )
    {
        Back->SetVisibility( bFaceUp ? ESlateVisibility::Collapsed : ESlateVisibility::Visible );
    }

    const bool bValueChanged = GetAssignReason() == ESmartTableAssignReason::ValueChanged;
    const bool bTurnedUp     = !bWasFaceUp && bFaceUp;
    const bool bTurnedDown   = bWasFaceUp && !bFaceUp;
    const bool bJustMatched  = bMatched && !bWasMatched;

    bWasFaceUp  = bFaceUp;
    bWasMatched = bMatched;

    if ( !bValueChanged )
    {
        return;
    }

    const FName Animation = bJustMatched ? MatchAnimation : ( bTurnedUp ? FlipAnimation : ( bTurnedDown ? MissAnimation : NAME_None ) );
    if ( Animation.IsNone() )
    {
        return;
    }

    UWidgetAnimation * Found = FindCellAnimation( Animation );

    UE_LOGFMT( LogSmartTables, Verbose, "Card {Row}/{Column} plays '{Animation}' ({Found}).", GetRowIndex(), GetColumnId(), Animation, Found ? TEXT( "authored" ) : TEXT( "NOT ON THE ASSET" ) );

    if ( Found )
    {
        PlayAnimation( Found );
    }
}

#undef LOCTEXT_NAMESPACE
