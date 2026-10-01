#include "SmartTableCells.h"
#include "SmartTableCellReads.h"
#include "SmartTableDemoConstants.h"

#include "Components/TextBlock.h"
#include "Logging/StructuredLog.h"
#include "Rendering/DrawElements.h"
#include "SmartTable.h"
#include "SmartTableLog.h"
#include "SmartTableModel.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SmartTables"

TSharedRef< SWidget > USmartTableSpinCell::RebuildWidget()
{

    return SNew( SBox )
        .VAlign( VAlign_Center )
        [
            SAssignNew( SpinBox, SSpinBox< double > )
                .IsEnabled( TAttribute< bool >::Create( TAttribute< bool >::FGetter::CreateUObject( this, &USmartTableSpinCell::IsInteractive ) ) )
                .OnValueChanged( SSpinBox< double >::FOnValueChanged::CreateUObject( this, &USmartTableSpinCell::HandleValueChanged ) )
        ];

}

double USmartTableSpinCell::ReadValue() const
{
    const TOptional< double > Value = SmartTableDemo::ReadNumber( *this );

    return Value.IsSet() ? Value.GetValue() : 0.0;
}

bool USmartTableSpinCell::IsInteractive() const
{
    const USmartTable * OwningTable = GetTable();

    return OwningTable && OwningTable->AreCellsInteractive();
}

void USmartTableSpinCell::HandleValueChanged( double NewValue )
{
    USmartTable * OwningTable = GetTable();
    if ( !OwningTable )
    {
        UE_LOGFMT( LogSmartTables, Verbose, "A spin cell changed with no table to report to; the edit is dropped." );
        return;
    }

    OwningTable->NotifyCellValueChanged( GetItem(), GetRowIndex(), GetColumnId(), static_cast< float >( NewValue ) );
}

void USmartTableSpinCell::NativeOnCellAssigned()
{
    Super::NativeOnCellAssigned();

    if ( const USmartTable * StyledTable = GetTable() )
    {
        SetColorAndOpacity( StyledTable->GetCellTextStyle().ColorAndOpacity.GetSpecifiedColor() );
    }

    const USmartTable * OwningTable  = GetTable();
    const FSmartTableColumn * Column = OwningTable ? OwningTable->FindColumn( GetColumnId() ) : nullptr;

    if ( SpinBox.IsValid() )
    {
        SpinBox->SetMinSliderValue( MinSliderValue );
        SpinBox->SetMaxSliderValue( MaxSliderValue );

        const int32 Digits = Column ? Column->MaxFractionalDigits : 2;
        SpinBox->SetMinFractionalDigits( Digits );
        SpinBox->SetMaxFractionalDigits( Digits );

        SpinBox->SetValue( ReadValue() );
    }
}

class SSmartTableAngleDial : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS( SSmartTableAngleDial )
        : _Size( 26.0f )
    {
    }
    SLATE_ARGUMENT( float, Size )
    SLATE_ATTRIBUTE( bool, Locked )
    SLATE_EVENT( FOnFloatValueChanged, OnAngleChanged )
    SLATE_END_ARGS()

    void Construct( const FArguments & InArgs )
    {
        Size           = InArgs._Size;
        Locked         = InArgs._Locked;
        OnAngleChanged = InArgs._OnAngleChanged;
    }

    void SetDegrees( TOptional< float > InDegrees )
    {
        Degrees = InDegrees;
    }

    virtual FVector2D ComputeDesiredSize( float ) const override
    {
        return FVector2D( Size, Size );
    }

    virtual FCursorReply OnCursorQuery( const FGeometry & Geometry, const FPointerEvent & CursorEvent ) const override
    {
        return Locked.Get( true ) ? FCursorReply::Unhandled() : FCursorReply::Cursor( EMouseCursor::Crosshairs );
    }

    virtual FReply OnMouseButtonDown( const FGeometry & Geometry, const FPointerEvent & MouseEvent ) override
    {
        if ( Locked.Get( true ) || MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton )
        {
            return FReply::Unhandled();
        }

        PointAt( Geometry, MouseEvent );

        return FReply::Handled().CaptureMouse( SharedThis( this ) );
    }

    virtual FReply OnMouseMove( const FGeometry & Geometry, const FPointerEvent & MouseEvent ) override
    {
        if ( !HasMouseCapture() )
        {
            return FReply::Unhandled();
        }

        PointAt( Geometry, MouseEvent );

        return FReply::Handled();
    }

    virtual FReply OnMouseButtonUp( const FGeometry & Geometry, const FPointerEvent & MouseEvent ) override
    {
        return HasMouseCapture() ? FReply::Handled().ReleaseMouseCapture() : FReply::Unhandled();
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

        const FLinearColor RingColour = InWidgetStyle.GetColorAndOpacityTint() * FLinearColor( 1.0f, 1.0f, 1.0f, 0.25f );

        TArray< FVector2D > Ring;
        Ring.Reserve( 25 );
        for ( int32 Step = 0; Step <= 24; ++Step )
        {
            const float Turn = ( Step / 24.0f ) * 2.0f * PI;
            Ring.Add( FVector2D( Centre.X + FMath::Sin( Turn ) * Radius, Centre.Y - FMath::Cos( Turn ) * Radius ) );
        }

        FSlateDrawElement::MakeLines( OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), Ring, ESlateDrawEffect::None, RingColour, true, 1.0f );

        if ( !Degrees.IsSet() )
        {
            return LayerId + 1;
        }

        const float Turn = FMath::DegreesToRadians( Degrees.GetValue() );
        const FVector2D Tip( Centre.X + FMath::Sin( Turn ) * Radius, Centre.Y - FMath::Cos( Turn ) * Radius );
        const FVector2D Tail( Centre.X - FMath::Sin( Turn ) * Radius * 0.35f, Centre.Y + FMath::Cos( Turn ) * Radius * 0.35f );

        const TArray< FVector2D > Needle = { Tail, Tip };
        FSlateDrawElement::MakeLines( OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), Needle, ESlateDrawEffect::None, InWidgetStyle.GetColorAndOpacityTint(), true, 2.0f );

        return LayerId + 2;
    }

private:
    void PointAt( const FGeometry & Geometry, const FPointerEvent & MouseEvent )
    {
        const FVector2f Local = FVector2f( Geometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ) );
        const FVector2f Delta = Local - Geometry.GetLocalSize() * 0.5f;
        if ( Delta.IsNearlyZero() )
        {
            return;
        }

        const float Bearing = FMath::Fmod( FMath::RadiansToDegrees( FMath::Atan2( Delta.X, -Delta.Y ) ) + 360.0f, 360.0f );

        Degrees = Bearing;

        OnAngleChanged.ExecuteIfBound( Bearing );
    }

    float Size = 26.0f;
    TOptional< float > Degrees;
    TAttribute< bool > Locked;
    FOnFloatValueChanged OnAngleChanged;
};

TSharedRef< SWidget > USmartTableProgressCell::RebuildWidget()
{

    return SNew( SVerticalBox )
        + SVerticalBox::Slot()
            .AutoHeight()
            .VAlign( VAlign_Center )
            [
                SAssignNew( Slider, SSlider )
                    .Locked( TAttribute< bool >::Create( TAttribute< bool >::FGetter::CreateUObject( this, &USmartTableProgressCell::IsLocked ) ) )
                    .OnValueChanged( FOnFloatValueChanged::CreateUObject( this, &USmartTableProgressCell::HandleSliderMoved ) )
            ]
        + SVerticalBox::Slot()
            .AutoHeight()
            [
                SAssignNew( ValueText, STextBlock )
            ];

}

float USmartTableProgressCell::ReadFraction() const
{
    const TOptional< double > Value = SmartTableDemo::ReadNumber( *this );
    const float Span                = MaxValue - MinValue;

    return Value.IsSet() && !FMath::IsNearlyZero( Span ) ? FMath::Clamp( static_cast< float >( Value.GetValue() - MinValue ) / Span, 0.0f, 1.0f ) : 0.0f;
}

bool USmartTableProgressCell::IsLocked() const
{
    const USmartTable * OwningTable = GetTable();

    return !OwningTable || !OwningTable->AreCellsInteractive();
}

void USmartTableProgressCell::HandleSliderMoved( float NewFraction )
{
    USmartTable * OwningTable = GetTable();
    if ( !OwningTable )
    {
        UE_LOGFMT( LogSmartTables, Verbose, "A progress cell moved with no table to report to; the edit is dropped." );
        return;
    }

    OwningTable->NotifyCellValueChanged( GetItem(), GetRowIndex(), GetColumnId(), MinValue + NewFraction * ( MaxValue - MinValue ) );
}

void USmartTableProgressCell::NativeOnCellAssigned()
{
    Super::NativeOnCellAssigned();

    if ( const USmartTable * StyledTable = GetTable() )
    {
        SetColorAndOpacity( StyledTable->GetCellTextStyle().ColorAndOpacity.GetSpecifiedColor() );
    }

    if ( Slider.IsValid() )
    {
        Slider->SetValue( ReadFraction() );
    }

    if ( ValueText.IsValid() )
    {
        const USmartTable * OwningTable = GetTable();
        if ( OwningTable )
        {
            ValueText->SetTextStyle( &OwningTable->GetCellTextStyle(), true );
        }

        ValueText->SetVisibility( bShowValue ? EVisibility::HitTestInvisible : EVisibility::Collapsed );
        ValueText->SetText( GetModel() ? GetModel()->GetCellText( GetRowIndex(), GetColumnId() ) : FText::GetEmpty() );
    }
}

TSharedRef< SWidget > USmartTableAngleCell::RebuildWidget()
{

    return SNew( SVerticalBox )
        + SVerticalBox::Slot()
            .AutoHeight()
            .HAlign( HAlign_Center )
            [
                SNew( SBox )
                    .WidthOverride( DialSize )
                    .HeightOverride( DialSize )
                    [
                        SAssignNew( Dial, SSmartTableAngleDial )
                            .Size( DialSize )
                            .Locked( TAttribute< bool >::Create( TAttribute< bool >::FGetter::CreateUObject( this, &USmartTableAngleCell::IsLocked ) ) )
                            .OnAngleChanged( FOnFloatValueChanged::CreateUObject( this, &USmartTableAngleCell::HandleAngleDragged ) )
                    ]
            ]
        + SVerticalBox::Slot()
            .AutoHeight()
            .HAlign( HAlign_Center )
            [
                SAssignNew( DegreesText, STextBlock )
            ];

}

bool USmartTableAngleCell::IsLocked() const
{
    const USmartTable * OwningTable = GetTable();

    return !OwningTable || !OwningTable->AreCellsInteractive();
}

void USmartTableAngleCell::HandleAngleDragged( float NewDegrees )
{
    USmartTable * OwningTable = GetTable();
    if ( !OwningTable )
    {
        UE_LOGFMT( LogSmartTables, Verbose, "An angle cell moved with no table to report to; the edit is dropped." );
        return;
    }

    OwningTable->NotifyCellValueChanged( GetItem(), GetRowIndex(), GetColumnId(), NewDegrees );
}

void USmartTableAngleCell::NativeOnCellAssigned()
{
    Super::NativeOnCellAssigned();

    if ( const USmartTable * StyledTable = GetTable() )
    {
        SetColorAndOpacity( StyledTable->GetCellTextStyle().ColorAndOpacity.GetSpecifiedColor() );
    }

    const TOptional< double > Value = SmartTableDemo::ReadNumber( *this );

    if ( Dial.IsValid() )
    {
        Dial->SetDegrees( Value.IsSet() ? TOptional< float >( static_cast< float >( Value.GetValue() ) ) : TOptional< float >() );
    }

    if ( DegreesText.IsValid() )
    {
        if ( const USmartTable * OwningTable = GetTable() )
        {
            DegreesText->SetTextStyle( &OwningTable->GetCellTextStyle(), true );
        }

        DegreesText->SetText( GetModel() ? GetModel()->GetCellText( GetRowIndex(), GetColumnId() ) : FText::GetEmpty() );
    }
}

TSharedRef< SWidget > USmartTableWrappedTextCell::RebuildWidget()
{
    return SAssignNew( WrappedText, STextBlock );
}

float USmartTableWrappedTextCell::GetWrapWidth() const
{
    const USmartTable * OwningTable = GetTable();
    if ( !OwningTable )
    {
        UE_LOGFMT( LogSmartTables, VeryVerbose, "Wrapped-text cell for '{Column}' has no table to measure against.", GetColumnId() );
        return 0.0f;
    }

    const float ColumnWidth = OwningTable->GetColumnWidth( GetColumnId() );
    if ( ColumnWidth <= 0.0f )
    {
        return 0.0f;
    }

    const FSmartTableColumn * Column = OwningTable->FindColumn( GetColumnId() );
    const float SidePadding          = Column ? Column->CellPadding.GetTotalSpaceAlong< Orient_Horizontal >() : 0.0f;

    return FMath::Max( ColumnWidth - SidePadding, 16.0f );
}

void USmartTableWrappedTextCell::NativeOnCellAssigned()
{
    Super::NativeOnCellAssigned();

    if ( !WrappedText.IsValid() )
    {
        UE_LOGFMT( LogSmartTables, VeryVerbose, "Wrapped-text cell for '{Column}' has no text block yet.", GetColumnId() );
        return;
    }

    FSlateColor StyleColour = FSlateColor::UseForeground();
    if ( const USmartTable * OwningTable = GetTable() )
    {
        WrappedText->SetTextStyle( &OwningTable->GetCellTextStyle(), true );
        StyleColour = OwningTable->GetCellTextStyle().ColorAndOpacity;

        const float WrapAt = GetWrapWidth();

        WrappedText->SetAutoWrapText( WrapAt <= 0.0f );
        WrappedText->SetWrapTextAt( WrapAt );
    }

    if ( MaxLines > 0 )
    {
        WrappedText->SetOverflowPolicy( ETextOverflowPolicy::Ellipsis );
    }

    USmartTableModel * CellModel = GetModel();
    WrappedText->SetText( CellModel ? CellModel->GetCellText( GetRowIndex(), GetColumnId() ) : FText::GetEmpty() );

    const FLinearColor Colour = CellModel ? CellModel->GetCellColor( GetRowIndex(), GetColumnId() ) : SmartTableDemo::NoColourOpinion();

    WrappedText->SetColorAndOpacity( Colour.A > 0.0f ? FSlateColor( Colour ) : StyleColour );
}

void USmartTablePulseTextCell::NativeOnCellAssigned()
{
    Super::NativeOnCellAssigned();

    if ( !ValueText )
    {
        if ( !bWarnedNoValueText )
        {
            bWarnedNoValueText = true;

            UE_LOGFMT( LogSmartTables, Warning, "Cell '{Column}' has no widget called ValueText, so it draws nothing. Add a Text Block called ValueText to the Widget Blueprint.", GetColumnId() );
        }

        return;
    }

    USmartTableModel * CellModel = GetModel();

    ValueText->SetText( CellModel ? CellModel->GetCellText( GetRowIndex(), GetColumnId() ) : FText::GetEmpty() );

    const USmartTable * StyledTable = GetTable();
    if ( !StyledTable )
    {
        return;
    }

    const FTextBlockStyle & Style = StyledTable->GetCellTextStyle();
    ValueText->SetFont( Style.Font );

    const FLinearColor Colour = CellModel ? CellModel->GetCellColor( GetRowIndex(), GetColumnId() ) : SmartTableDemo::NoColourOpinion();

    ValueText->SetColorAndOpacity( Colour.A > 0.0f ? FSlateColor( Colour ) : Style.ColorAndOpacity );
}

#undef LOCTEXT_NAMESPACE
