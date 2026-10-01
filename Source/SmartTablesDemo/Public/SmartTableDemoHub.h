#pragma once

#include "Blueprint/UserWidget.h"
#include "Engine/TimerHandle.h"
#include "SmartTableTypes.h"
#include "SmartTableDemoHub.generated.h"

class UButton;
class UDataTable;
class USmartTable;
class UWidgetSwitcher;

UCLASS( Abstract )
class SMARTTABLESDEMO_API USmartTableDemoHub : public UUserWidget
{
    GENERATED_BODY()

public:
    class USmartTableDemoPage * GetCurrentPage() const
    {
        return CurrentPage;
    }

    void OpenDemoByIndex( int32 DemoIndex );

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Smart Tables|Demo", meta = ( ToolTip = "" ) )
    TSoftObjectPtr< UDataTable > DemoDataTable;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Smart Tables|Demo", meta = ( ToolTip = "", ClampMin = "1" ) )
    int32 HugeDemoRowCount = 1000000;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Smart Tables|Demo", meta = ( ToolTip = "", ClampMin = "0.0" ) )
    float AsyncDemoLatencySeconds = 1.5f;

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Demo|Pages", meta = ( ToolTip = "" ) )
    TSoftClassPtr< class USmartTableDemoPage > ItemsPage;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Smart Tables|Demo", meta = ( ToolTip = "" ) )
    TSoftClassPtr< class USmartTableShowcaseScreen > PairsScreen;

    UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Smart Tables|Demo", meta = ( ToolTip = "" ) )
    TSoftClassPtr< class USmartTableShowcaseScreen > ServerBrowserScreen;

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Demo|Pages", meta = ( ToolTip = "" ) )
    TSoftClassPtr< class USmartTableDataTablePage > DataTablePage;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo", meta = ( ToolTip = "" ) )
    TSoftClassPtr< class USmartTableRecordFormatPage > CsvPage;

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo", meta = ( ToolTip = "" ) )
    TSoftClassPtr< class USmartTableRecordFormatPage > JsonPage;

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Demo|Pages", meta = ( ToolTip = "" ) )
    TSoftClassPtr< class USmartTableDemoPage > MillionPage;

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Demo|Pages", meta = ( ToolTip = "" ) )
    TSoftClassPtr< class USmartTableDemoPage > KeyboardInputPage;

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Demo|Pages", meta = ( ToolTip = "" ) )
    TSoftClassPtr< class USmartTableDemoPage > MvvmItemsPage;

    UPROPERTY( EditAnywhere, Category = "Smart Tables|Demo", meta = ( ToolTip = "", ClampMin = "0.0" ) )
    float StationFeedSeconds = 0.4f;

protected:
    virtual void NativeOnInitialized() override;

private:
    UFUNCTION()
    void ShowItemDemo();

    UFUNCTION()
    void ShowDataTableDemo();
    void ShowCsvDemo();
    void ShowJsonDemo();

    void OpenRecordFormatDemo( const TSoftClassPtr< class USmartTableRecordFormatPage > & PageClass, const FText & Title );

    UFUNCTION()
    void ShowHugeDemo();

    void BuildAsyncDemo( const TSoftClassPtr< class USmartTableDemoPage > & PageClass, int32 RowCount, const FText & Title );

    UFUNCTION()
    void HandleMenuRowChosen( UObject * Item, int32 NaturalRow, FName ColumnId, float Value );

    UFUNCTION()
    void ShowMenu();

    void ShowKeyboardInputDemo();

    void ShowMvvmItemDemo();

    void NudgeStation();

    void BuildItemDemo( const TSoftClassPtr< class USmartTableDemoPage > & PageClass, const FText & Title );

    UFUNCTION()
    void HandleSelectionForBanner( const TArray< int32 > & SelectedRows );

    UFUNCTION()
    void ShowServerScreen();

    UFUNCTION()
    void ShowPairsScreen();

    void ShowOffscreenDemo();

    void ShowScreen( TSubclassOf< class USmartTableShowcaseScreen > ScreenClass );

    void FillMenuTable();

    void BuildLayout();

    void TakeDownCurrent();

    bool OpenDemoPage( const TSoftClassPtr< class USmartTableDemoPage > & PageClass );

    void BeginDemo( const FText & Title );

    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidget, AllowPrivateAccess = "true" ) )
    TObjectPtr< UWidgetSwitcher > Pages;

    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidget, AllowPrivateAccess = "true" ) )
    TObjectPtr< USmartTable > MenuTable;

    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidget, AllowPrivateAccess = "true" ) )
    TObjectPtr< class UBorder > TableHost;

    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidgetOptional, AllowPrivateAccess = "true" ) )
    TObjectPtr< class UBorder > PageActionsHost;

    UPROPERTY( Transient )
    TObjectPtr< class USmartTableDemoPage > CurrentPage;

    UPROPERTY( Transient )
    TObjectPtr< USmartTable > Table;

    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidget, AllowPrivateAccess = "true" ) )
    TObjectPtr< class UBorder > ScreenHost;

    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidget, AllowPrivateAccess = "true" ) )
    TObjectPtr< class UTextBlock > SelectionBanner;

    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidget, AllowPrivateAccess = "true" ) )
    TObjectPtr< class UTextBlock > TitleText;

    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidget, AllowPrivateAccess = "true" ) )
    TObjectPtr< UButton > BackButton;

    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidget, AllowPrivateAccess = "true" ) )
    TObjectPtr< UButton > ScreenBackButton;

    UPROPERTY( Transient )
    TObjectPtr< class USmartTableShowcaseScreen > CurrentScreen;

    UPROPERTY( Transient )
    TObjectPtr< class ASmartTableOffscreenStage > OffscreenStage;

    UPROPERTY( Transient )
    TObjectPtr< class USmartTableAsyncDemoModel > AsyncModel;

    FTimerHandle StationFeed;
};
