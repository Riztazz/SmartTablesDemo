#pragma once

#include "SmartTableCell.h"
#include "SmartTableTypes.h"
#include "SmartTableShowcaseCells.generated.h"

UCLASS()
class SMARTTABLESDEMO_API USmartTableChipCell : public USmartTableCell
{
    GENERATED_BODY()

protected:
    virtual void NativeOnCellAssigned() override;
    virtual TSharedRef< SWidget > RebuildWidget() override;

private:
    TSharedPtr< class SSmartTableChip > Chip;
};

UCLASS()
class SMARTTABLESDEMO_API USmartTableSignalCell : public USmartTableCell
{
    GENERATED_BODY()

protected:
    virtual void NativeOnCellAssigned() override;
    virtual TSharedRef< SWidget > RebuildWidget() override;

private:
    TSharedPtr< class SSmartTableSignal > Signal;
};

UCLASS()
class SMARTTABLESDEMO_API USmartTablePipCell : public USmartTableCell
{
    GENERATED_BODY()

protected:
    virtual void NativeOnCellAssigned() override;
    virtual TSharedRef< SWidget > RebuildWidget() override;

private:
    TSharedPtr< class SSmartTablePip > Pip;
};

UCLASS()
class SMARTTABLESDEMO_API USmartTableStackedCell : public USmartTableCell
{
    GENERATED_BODY()

protected:
    virtual void NativeOnCellAssigned() override;
    virtual TSharedRef< SWidget > RebuildWidget() override;

private:
    TSharedPtr< class STextBlock > TitleText;
    TSharedPtr< class STextBlock > SubtitleText;
};

UCLASS()
class SMARTTABLESDEMO_API USmartTableSlotCell : public USmartTableCell
{
    GENERATED_BODY()

protected:
    virtual void NativeOnCellAssigned() override;
    virtual TSharedRef< SWidget > RebuildWidget() override;

private:
    TSharedPtr< class SSmartTableSlot > Slot;

    TSharedPtr< class STextBlock > CountText;
};

UCLASS( Abstract )
class SMARTTABLESDEMO_API USmartTableCardCell : public USmartTableCell
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Pairs" )
    void Flip();

protected:
    virtual void NativeOnCellAssigned() override;
    virtual FReply NativeOnMouseButtonDown( const FGeometry & Geometry, const FPointerEvent & MouseEvent ) override;

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Pairs", meta = ( ToolTip = "" ) )
    FName FlipAnimation = TEXT( "Flip" );

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Pairs", meta = ( ToolTip = "" ) )
    FName MatchAnimation = TEXT( "Match" );

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Pairs", meta = ( ToolTip = "" ) )
    FName MissAnimation = TEXT( "Miss" );

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< class UTextBlock > SymbolText;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< class UBorder > Face;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< class UBorder > Back;

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Pairs", meta = ( ToolTip = "" ) )
    FName DissolveParameter = TEXT( "Dissolve" );

private:
    bool bWasFaceUp  = false;
    bool bWasMatched = false;
};
