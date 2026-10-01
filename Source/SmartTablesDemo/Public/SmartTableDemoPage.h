#pragma once

#include "Blueprint/UserWidget.h"
#include "SmartTableDemoPage.generated.h"

class UButton;
class UInputAction;
class UInputMappingContext;
class USmartTable;
class UWidget;

UCLASS( Abstract )
class SMARTTABLESDEMO_API USmartTableDemoPage : public UUserWidget
{
    GENERATED_BODY()

public:
    USmartTableDemoPage( const FObjectInitializer & ObjectInitializer );

    USmartTable * GetTable() const
    {
        return Table;
    }

    UWidget * GetPageActions() const
    {
        return PageActions;
    }

    UButton * GetAddRowButton() const
    {
        return AddRowButton;
    }

    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Demo" )
    void DumpRowsToCsv() const;

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Demo", meta = ( ToolTip = "", ClampMin = "1" ) )
    int32 SampleRowCount = 24;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo", meta = ( ToolTip = "" ) )
    TSoftClassPtr< UObject > SampleRowClass;

    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Demo" )
    TArray< UObject * > MakeSampleRows();

    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Demo" )
    UObject * MakeSampleRow( int32 Index );

    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Demo" )
    UObject * CopySampleRow( UObject * Source );

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", ClampMin = "1.0" ) )
    float ResizeStepPixels = 8.0f;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "" ) )
    bool bDemonstrateHeldResize = false;

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", ClampMin = "1.0" ) )
    float ResizeHoldPixelsPerSecond = 220.0f;

    void HandleFocus();
    void HandleUp();
    void HandleDown();
    void HandleActivate();
    void HandleMenu();
    void HandleSort();

    void HandleMoveRowUp();
    void HandleMoveRowDown();

    void StepResize( float DeltaPixels );

    void HandleHoldBegun();
    void HandleHoldStep();
    void HandleHoldEnded();

    void HandleWider();
    void HandleNarrower();

    UFUNCTION()
    void HandleCellValueChanged( UObject * Item, int32 NaturalRow, FName ColumnId, float Value );
    void HandleNextColumn();

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", AllowPrivateAccess = "true" ) )
    TObjectPtr< UInputMappingContext > InputContext;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", AllowPrivateAccess = "true" ) )
    TObjectPtr< UInputAction > FocusAction;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", AllowPrivateAccess = "true" ) )
    TObjectPtr< UInputAction > UpAction;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", AllowPrivateAccess = "true" ) )
    TObjectPtr< UInputAction > DownAction;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", AllowPrivateAccess = "true" ) )
    TObjectPtr< UInputAction > ActivateAction;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", AllowPrivateAccess = "true" ) )
    TObjectPtr< UInputAction > MenuAction;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", AllowPrivateAccess = "true" ) )
    TObjectPtr< UInputAction > SortAction;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", AllowPrivateAccess = "true" ) )
    TObjectPtr< UInputAction > WiderAction;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", AllowPrivateAccess = "true" ) )
    TObjectPtr< UInputAction > NarrowerAction;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", AllowPrivateAccess = "true" ) )
    TObjectPtr< UInputAction > NextColumnAction;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", AllowPrivateAccess = "true" ) )
    TObjectPtr< UInputAction > ResizeHoldAction;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", AllowPrivateAccess = "true" ) )
    TObjectPtr< UInputAction > MoveRowUpAction;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo|Input", meta = ( ToolTip = "", AllowPrivateAccess = "true" ) )
    TObjectPtr< UInputAction > MoveRowDownAction;

private:
    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidget, AllowPrivateAccess = "true" ) )
    TObjectPtr< USmartTable > Table;

    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidgetOptional, AllowPrivateAccess = "true" ) )
    TObjectPtr< UWidget > PageActions;

    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidgetOptional, AllowPrivateAccess = "true" ) )
    TObjectPtr< UButton > AddRowButton;
};

UCLASS( Abstract )
class SMARTTABLESDEMO_API USmartTableKeyboardDemoPage : public USmartTableDemoPage
{
    GENERATED_BODY()

public:
    USmartTableKeyboardDemoPage( const FObjectInitializer & ObjectInitializer );

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown( const FGeometry & Geometry, const FKeyEvent & KeyEvent ) override;
};
