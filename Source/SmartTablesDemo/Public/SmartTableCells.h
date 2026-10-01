#pragma once

#include "SmartTableCell.h"
#include "Styling/SlateTypes.h"
#include "SmartTableCells.generated.h"

template< typename NumericType >
class SSpinBox;

class SSmartTableAngleDial;
class SSlider;
class STextBlock;
class UTextBlock;

UCLASS( DisplayName = "Smart Table Progress Cell" )
class SMARTTABLESDEMO_API USmartTableProgressCell : public USmartTableCell
{
    GENERATED_BODY()

public:
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Smart Tables|Cell", meta = ( ToolTip = "" ) )
    float MinValue = 0.0f;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Smart Tables|Cell", meta = ( ToolTip = "" ) )
    float MaxValue = 100.0f;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Smart Tables|Cell", meta = ( ToolTip = "" ) )
    bool bShowValue = true;

protected:
    virtual void NativeOnCellAssigned() override;
    virtual TSharedRef< SWidget > RebuildWidget() override;

private:
    float ReadFraction() const;
    bool IsLocked() const;
    void HandleSliderMoved( float NewFraction );

    TSharedPtr< SSlider > Slider;
    TSharedPtr< STextBlock > ValueText;
};

UCLASS( DisplayName = "Smart Table Spin Cell" )
class SMARTTABLESDEMO_API USmartTableSpinCell : public USmartTableCell
{
    GENERATED_BODY()

public:
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Smart Tables|Cell", meta = ( ToolTip = "" ) )
    float MinSliderValue = 0.0f;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Smart Tables|Cell", meta = ( ToolTip = "" ) )
    float MaxSliderValue = 100.0f;

protected:
    virtual void NativeOnCellAssigned() override;
    virtual TSharedRef< SWidget > RebuildWidget() override;

private:
    double ReadValue() const;
    bool IsInteractive() const;
    void HandleValueChanged( double NewValue );

    TSharedPtr< SSpinBox< double > > SpinBox;
};

UCLASS( DisplayName = "Smart Table Angle Cell" )
class SMARTTABLESDEMO_API USmartTableAngleCell : public USmartTableCell
{
    GENERATED_BODY()

public:
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Smart Tables|Cell", meta = ( ToolTip = "", ClampMin = "8.0" ) )
    float DialSize = 78.0f;

protected:
    virtual void NativeOnCellAssigned() override;
    virtual TSharedRef< SWidget > RebuildWidget() override;

private:
    bool IsLocked() const;
    void HandleAngleDragged( float NewDegrees );

    TSharedPtr< SSmartTableAngleDial > Dial;
    TSharedPtr< STextBlock > DegreesText;
};

UCLASS( DisplayName = "Smart Table Wrapped Text Cell" )
class SMARTTABLESDEMO_API USmartTableWrappedTextCell : public USmartTableCell
{
    GENERATED_BODY()

public:
    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Smart Tables|Cell", meta = ( ToolTip = "", ClampMin = "0" ) )
    int32 MaxLines = 0;

protected:
    virtual void NativeOnCellAssigned() override;
    virtual TSharedRef< SWidget > RebuildWidget() override;

private:
    float GetWrapWidth() const;

    TSharedPtr< STextBlock > WrappedText;
};

UCLASS( DisplayName = "Smart Table Pulse Text Cell" )
class SMARTTABLESDEMO_API USmartTablePulseTextCell : public USmartTableCell
{
    GENERATED_BODY()

protected:
    virtual void NativeOnCellAssigned() override;

private:
    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< UTextBlock > ValueText;

    bool bWarnedNoValueText = false;
};
