#include "SmartTableDemoMenu.h"

#include "Framework/Application/SlateApplication.h"
#include "Logging/StructuredLog.h"
#include "Rendering/DrawElements.h"
#include "SmartTable.h"
#include "SmartTableLog.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SCompoundWidget.h"

namespace
{
    class SSmartTablePlayButton : public SCompoundWidget
    {
    public:
        DECLARE_DELEGATE( FOnPlayClicked )

        SLATE_BEGIN_ARGS( SSmartTablePlayButton )
            : _Size( 30.0f )
        {
        }
        SLATE_ARGUMENT( float, Size )
        SLATE_EVENT( FOnPlayClicked, OnClicked )
        SLATE_END_ARGS()

        void Construct( const FArguments & InArgs )
        {
            Size      = InArgs._Size;
            OnClicked = InArgs._OnClicked;

            SetCursor( EMouseCursor::Hand );
        }

        virtual FVector2D ComputeDesiredSize( float ) const override
        {
            return FVector2D( Size, Size );
        }

        virtual FReply OnMouseButtonDown( const FGeometry & Geometry, const FPointerEvent & MouseEvent ) override
        {
            return MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton ? FReply::Handled() : FReply::Unhandled();
        }

        virtual FReply OnMouseButtonUp( const FGeometry & Geometry, const FPointerEvent & MouseEvent ) override
        {
            if ( MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton )
            {
                return FReply::Unhandled();
            }

            if ( Geometry.IsUnderLocation( MouseEvent.GetScreenSpacePosition() ) )
            {
                OnClicked.ExecuteIfBound();
            }

            return FReply::Handled();
        }

        virtual int32 OnPaint( const FPaintArgs & Args, const FGeometry & AllottedGeometry, const FSlateRect & CullingRect, FSlateWindowElementList & OutDrawElements, int32 LayerId, const FWidgetStyle & InWidgetStyle, bool bParentEnabled ) const override
        {
            const FVector2f Local  = AllottedGeometry.GetLocalSize();
            const FVector2f Centre = Local * 0.5f;
            const float Radius     = FMath::Min( Local.X, Local.Y ) * 0.5f - 1.0f;
            if ( Radius <= 0.0f )
            {
                return LayerId;
            }

            const bool bHot               = IsHovered();
            const FLinearColor Tint       = InWidgetStyle.GetColorAndOpacityTint();
            const FLinearColor RingColour = Tint * FLinearColor( 1.0f, 1.0f, 1.0f, bHot ? 0.95f : 0.45f );

            const FSlateBrush * White         = FCoreStyle::Get().GetBrush( "WhiteBrush" );
            const FSlateResourceHandle Handle = FSlateApplication::Get().GetRenderer()->GetResourceHandle( *White );

            auto Vertex = [ &AllottedGeometry ]( float X, float Y, const FColor & Colour )
            {
                FSlateVertex V;
                V.Position       = FVector2f( AllottedGeometry.LocalToAbsolute( FVector2f( X, Y ) ) );
                V.Color          = Colour;
                V.TexCoords[ 0 ] = 0.5f;
                V.TexCoords[ 1 ] = 0.5f;
                V.TexCoords[ 2 ] = 0.5f;
                V.TexCoords[ 3 ] = 0.5f;

                return V;
            };

            const FVector2f AbsoluteOrigin = FVector2f( AllottedGeometry.LocalToAbsolute( FVector2f( 0.0f, 0.0f ) ) );
            const FVector2f AbsoluteUnitX  = FVector2f( AllottedGeometry.LocalToAbsolute( FVector2f( 1.0f, 0.0f ) ) );
            const float Feather            = 1.0f / FMath::Max( ( AbsoluteUnitX - AbsoluteOrigin ).Size(), KINDA_SMALL_NUMBER );

            const float HalfThickness    = ( bHot ? 2.0f : 1.5f ) * 0.5f;
            const float RadiusOuterEdge  = Radius + HalfThickness + Feather;
            const float RadiusOuterSolid = Radius + HalfThickness;
            const float RadiusInnerSolid = FMath::Max( Radius - HalfThickness, 0.0f );
            const float RadiusInnerEdge  = FMath::Max( RadiusInnerSolid - Feather, 0.0f );

            const FColor RingSolid = RingColour.ToFColor( false );
            const FColor RingClear = ( RingColour * FLinearColor( 1.0f, 1.0f, 1.0f, 0.0f ) ).ToFColor( false );

            constexpr int32 Segments = 64;

            TArray< FSlateVertex > RingVerts;
            RingVerts.Reserve( Segments * 4 );

            TArray< SlateIndex > RingIndices;
            RingIndices.Reserve( Segments * 18 );

            for ( int32 Step = 0; Step < Segments; ++Step )
            {
                const float Turn    = ( Step / static_cast< float >( Segments ) ) * 2.0f * PI;
                const float SinTurn = FMath::Sin( Turn );
                const float CosTurn = FMath::Cos( Turn );

                auto At = [ &Vertex, &Centre, SinTurn, CosTurn ]( float R, const FColor & Colour )
                {
                    return Vertex( Centre.X + SinTurn * R, Centre.Y - CosTurn * R, Colour );
                };

                RingVerts.Add( At( RadiusOuterEdge, RingClear ) );
                RingVerts.Add( At( RadiusOuterSolid, RingSolid ) );
                RingVerts.Add( At( RadiusInnerSolid, RingSolid ) );
                RingVerts.Add( At( RadiusInnerEdge, RingClear ) );

                const SlateIndex Here = static_cast< SlateIndex >( Step * 4 );
                const SlateIndex Next = static_cast< SlateIndex >( ( ( Step + 1 ) % Segments ) * 4 );

                for ( SlateIndex Band = 0; Band < 3; ++Band )
                {
                    RingIndices.Add( Here + Band );
                    RingIndices.Add( Next + Band );
                    RingIndices.Add( Here + Band + 1 );

                    RingIndices.Add( Next + Band );
                    RingIndices.Add( Next + Band + 1 );
                    RingIndices.Add( Here + Band + 1 );
                }
            }

            FSlateDrawElement::MakeCustomVerts( OutDrawElements, LayerId, Handle, RingVerts, RingIndices, nullptr, 0, 0 );

            const float Reach = Radius * 0.52f;
            const float Back  = Reach * 0.30f;
            const float Tip   = Reach * 0.60f;
            const float Half  = Reach * 0.62f;

            const FLinearColor Fill = Tint * FLinearColor( 1.0f, 1.0f, 1.0f, bHot ? 1.0f : 0.75f );
            const FColor Packed     = Fill.ToFColor( false );

            TArray< FSlateVertex > Verts;
            Verts.Add( Vertex( Centre.X - Back, Centre.Y - Half, Packed ) );
            Verts.Add( Vertex( Centre.X - Back, Centre.Y + Half, Packed ) );
            Verts.Add( Vertex( Centre.X + Tip, Centre.Y, Packed ) );

            const TArray< SlateIndex > Indices = { 0, 1, 2 };

            FSlateDrawElement::MakeCustomVerts( OutDrawElements, LayerId + 1, Handle, Verts, Indices, nullptr, 0, 0 );

            return LayerId + 2;
        }

    private:
        float Size = 30.0f;
        FOnPlayClicked OnClicked;
    };
}
TSharedRef< SWidget > USmartTableDemoPlayCell::RebuildWidget()
{

    return SNew( SSmartTablePlayButton )
        .Size( 30.0f )
        .OnClicked_Lambda( [ this ]()
            {
                USmartTable * OwningTable = GetTable();
                if ( !OwningTable )
                {
                    UE_LOGFMT( LogSmartTables, Verbose, "A play button was clicked with no table to report to; the demo cannot start." );
                    return;
                }

                OwningTable->NotifyCellValueChanged( GetItem(), GetRowIndex(), GetColumnId(), 1.0f );
            } );

}
