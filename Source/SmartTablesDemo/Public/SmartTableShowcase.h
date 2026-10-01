#pragma once

#include "Blueprint/UserWidget.h"
#include "SmartTableShowcase.generated.h"

class USmartTable;
class USmartTableStyle;
class USmartTableInventoryModel;
class USmartTableFeedModel;
class USmartTableServerModel;
class UTextBlock;
class UVerticalBox;

UCLASS( Abstract )
class SMARTTABLESDEMO_API USmartTableShowcaseScreen : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Showcase" )
    void BuildScreen();

    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Showcase", meta = ( WorldContext = "WorldContextObject" ) )
    static USmartTableShowcaseScreen * ShowScreenOfClass( UObject * WorldContextObject, TSubclassOf< USmartTableShowcaseScreen > ScreenClass );

protected:
    virtual void NativeOnInitialized() override;

    virtual void ConfigureScreen()
    {
    }

    struct FPalette
    {
        FLinearColor Backdrop;
        FLinearColor Panel;
        FLinearColor RowEven;
        FLinearColor RowOdd;
        FLinearColor Hover;
        FLinearColor Selected;
        FLinearColor Text;
        FLinearColor Muted;
        FLinearColor Accent;
        FLinearColor Rule;
    };

    FPalette PaletteFromStyle( const USmartTableStyle & InStyle ) const;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Showcase", meta = ( ToolTip = "" ) )
    TObjectPtr< USmartTableStyle > ScreenStyle;

private:
    bool bScreenBuilt = false;

protected:
    UTextBlock * AddHeading( UVerticalBox * Parent, const FText & Text, int32 Size, const FLinearColor & Colour, const FMargin & InPadding );

    class UBorder * MakePanel( const FLinearColor & Colour, const FMargin & InPadding );

    UTextBlock * MakeLabel( const FText & Text, int32 Size, const FLinearColor & Colour );

    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Showcase", meta = ( BindWidgetOptional ) )
    TObjectPtr< USmartTable > Table;
};

UCLASS()
class SMARTTABLESDEMO_API USmartTableServerScreen : public USmartTableShowcaseScreen
{
    GENERATED_BODY()

public:
    USmartTableServerScreen( const FObjectInitializer & ObjectInitializer );

    void BurstFeed( int32 Count );

protected:
    virtual void ConfigureScreen() override;
    virtual void NativeDestruct() override;

private:
    UFUNCTION()
    void HandleFilterChanged( const FText & Text );

    UFUNCTION()
    void HandleSelectionChanged( const TArray< int32 > & SelectedRows );

    void ShowDetail( int32 NaturalRow );

    void PushEntry();

    UPROPERTY( Transient )
    TObjectPtr< USmartTableServerModel > Model;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< class UBorder > Backdrop;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< UTextBlock > TitleText;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< UTextBlock > SubtitleText;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< UTextBlock > FeedTitle;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< USmartTable > LoadoutTable;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< UTextBlock > LoadoutTitle;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< class UEditableTextBox > FilterBox;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< class UBorder > FeedPanel;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< class UBorder > DetailPanel;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< UVerticalBox > DetailBox;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< USmartTable > FeedTable;

    UPROPERTY( Transient )
    TObjectPtr< USmartTableFeedModel > FeedModel;

    UPROPERTY( Transient )
    TObjectPtr< class USmartTableInventoryModel > LoadoutModel;

    static constexpr int32 LoadoutColumns  = 3;
    static constexpr int32 LoadoutRows     = 3;
    static constexpr float LoadoutSlotSize = 92.0f;

    void ConfigureLoadout( const FPalette & Palette );

    UFUNCTION()
    void HandleSlotDropped( class USmartTableCellDragDropOp * Payload, int32 TargetRow, FName TargetColumnId );

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Showcase", meta = ( ToolTip = "" ) )
    TObjectPtr< USmartTableStyle > FeedStyle;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Showcase", meta = ( ToolTip = "", ClampMin = "0.05" ) )
    float FeedIntervalSeconds = 1.1f;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Showcase", meta = ( ToolTip = "", ClampMin = "1" ) )
    int32 MaxFeedRows = 24;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< UTextBlock > CounterText;

    FTimerHandle FeedTimer;
};

UCLASS()
class SMARTTABLESDEMO_API USmartTablePairsScreen : public USmartTableShowcaseScreen
{
    GENERATED_BODY()

public:
    USmartTablePairsScreen( const FObjectInitializer & ObjectInitializer );

    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Pairs" )
    void FlipAt( int32 Row, int32 Column );

protected:
    virtual void ConfigureScreen() override;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< class UBorder > Backdrop;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< UTextBlock > TitleText;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< UTextBlock > SubtitleText;

    UPROPERTY( meta = ( BindWidgetOptional ) )
    TObjectPtr< UTextBlock > ScoreText;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Showcase", meta = ( ToolTip = "" ) )
    TSoftClassPtr< class USmartTableCell > CardCellClass;

    UPROPERTY( Transient )
    TObjectPtr< class USmartTablePairsModel > Model;

private:
    UFUNCTION()
    void HandleTurn( ESmartTablePairsTurn Turn );

    void ShowScore();

    static constexpr int32 BoardColumns = 6;
    static constexpr int32 BoardRows    = 5;

    static constexpr float CardSize = 160.0f;

    static constexpr float MissSeconds = 0.9f;

    FTimerHandle TurnBackTimer;
};
