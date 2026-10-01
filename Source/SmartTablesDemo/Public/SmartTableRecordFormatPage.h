#pragma once

#include "SmartTableDemoPage.h"
#include "SmartTableRecordFormatPage.generated.h"

class UButton;
class USmartTableRecordModel;
class UTextBlock;

UCLASS( Abstract )
class SMARTTABLESDEMO_API USmartTableRecordFormatPage : public USmartTableDemoPage
{
    GENERATED_BODY()

public:
    void BeginRecordFormat();

    bool SaveNow();

protected:
    virtual void NativeOnInitialized() override;

    UFUNCTION()
    void HandleSaveClicked();

    UPROPERTY( EditDefaultsOnly, Category = "Smart Tables|Demo", meta = ( ToolTip = "" ) )
    FString ContentRelativePath = TEXT( "Demo/Csv/Strings.csv" );

private:
    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidgetOptional, AllowPrivateAccess = "true" ) )
    TObjectPtr< UButton > SaveButton;

    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidgetOptional, AllowPrivateAccess = "true" ) )
    TObjectPtr< UTextBlock > StatusText;

    UPROPERTY( Transient )
    TObjectPtr< USmartTableRecordModel > Model;
};
