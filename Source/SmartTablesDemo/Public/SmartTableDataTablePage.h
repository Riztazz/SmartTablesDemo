#pragma once

#include "SmartTableDemoPage.h"
#include "SmartTableDataTablePage.generated.h"

class UDataTable;
class USmartTableDemoDataTableModel;
class UTextBlock;

UCLASS( Abstract )
class SMARTTABLESDEMO_API USmartTableDataTablePage : public USmartTableDemoPage
{
    GENERATED_BODY()

public:
    void BeginDataTable( UDataTable * InDataTable );

private:
    UPROPERTY( BlueprintReadOnly, Category = "Smart Tables|Demo", meta = ( BindWidgetOptional, AllowPrivateAccess = "true" ) )
    TObjectPtr< UTextBlock > NoteText;

    UPROPERTY( Transient )
    TObjectPtr< USmartTableDemoDataTableModel > Model;
};
