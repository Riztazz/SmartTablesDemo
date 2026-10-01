#pragma once

#include "SmartTableModel.h"
#include "SmartTableShowcaseModels.generated.h"

UENUM()
enum class ESmartTableFeedSeverity : uint8
{
    Trace,
    Info,
    Warning,
    Critical
};

UCLASS()
class SMARTTABLESDEMO_API USmartTableFeedModel : public USmartTableModel
{
    GENERATED_BODY()

public:
    void Prime( int32 NumEntries );

    void Append();

    void DropOldest();

    int32 GetCountAtOrAbove( ESmartTableFeedSeverity Minimum ) const;

protected:
    virtual int32 GetNumRows_Implementation() override;
    virtual FText GetCellText_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FLinearColor GetCellColor_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FLinearColor GetRowColor_Implementation( int32 NaturalRow ) override;
    virtual FSmartTableSortKey GetCellSortKey_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FName GetRowId_Implementation( int32 NaturalRow ) override;

private:
    struct FEntry
    {
        FString Source;
        FString Message;
        ESmartTableFeedSeverity Severity = ESmartTableFeedSeverity::Info;
        int32 Tick                       = 0;
    };

    TArray< FEntry > Entries;
    int32 Clock = 0;
};

UCLASS()
class SMARTTABLESDEMO_API USmartTableServerModel : public USmartTableModel
{
    GENERATED_BODY()

public:
    void GenerateServers( int32 NumServers );

    bool IsLocked( int32 NaturalRow ) const;

    FText GetServerName( int32 NaturalRow ) const;

protected:
    virtual int32 GetNumRows_Implementation() override;
    virtual FText GetCellText_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FLinearColor GetCellColor_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FSmartTableSortKey GetCellSortKey_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FName GetRowId_Implementation( int32 NaturalRow ) override;

private:
    struct FServer
    {
        FString Name;
        FString Region;
        FString Mode;
        FString Map;
        int32 Players  = 0;
        int32 Slots    = 0;
        int32 PingMs   = 0;
        bool bLocked   = false;
        bool bOfficial = false;
    };

    TArray< FServer > Servers;
};

UCLASS()
class SMARTTABLESDEMO_API USmartTableInventoryModel : public USmartTableModel
{
    GENERATED_BODY()

public:
    void GenerateBag( int32 InColumns, int32 InRows );

    TArray< FName > GetSlotColumnIds() const;

    FText GetItemName( int32 NaturalRow, FName ColumnId ) const;

    FText GetItemRarity( int32 NaturalRow, FName ColumnId ) const;

    void SwapSlots( int32 RowA, FName ColumnA, int32 RowB, FName ColumnB );

protected:
    virtual int32 GetNumRows_Implementation() override;
    virtual FText GetCellText_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FLinearColor GetCellColor_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FSmartTableSortKey GetCellSortKey_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FName GetRowId_Implementation( int32 NaturalRow ) override;

private:
    struct FSlot
    {
        FString Name;
        int32 Shape  = INDEX_NONE;
        int32 Rarity = 0;
        int32 Count  = 0;

        bool IsEmpty() const
        {
            return Shape == INDEX_NONE;
        }
    };

    int32 IndexOfColumn( FName ColumnId ) const;
    const FSlot * FindSlot( int32 NaturalRow, FName ColumnId ) const;

    int32 Columns = 0;
    TArray< TArray< FSlot > > Grid;
};

UENUM( BlueprintType )
enum class ESmartTablePairsTurn : uint8
{
    Ignored,

    Revealed,

    Matched,

    Missed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam( FOnSmartTablePairsTurn, ESmartTablePairsTurn, Turn );

UCLASS()
class SMARTTABLESDEMO_API USmartTablePairsModel : public USmartTableModel
{
    GENERATED_BODY()

public:
    void Deal( int32 InColumns, int32 InRows );

    TArray< FName > GetCardColumnIds() const;

    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Pairs" )
    void FlipCard( int32 NaturalRow, FName ColumnId );

    UFUNCTION( BlueprintPure, Category = "Smart Tables|Pairs" )
    bool IsFaceUp( int32 NaturalRow, FName ColumnId ) const;

    UFUNCTION( BlueprintPure, Category = "Smart Tables|Pairs" )
    bool IsMatched( int32 NaturalRow, FName ColumnId ) const;

    UFUNCTION( BlueprintPure, Category = "Smart Tables|Pairs" )
    int32 GetSymbol( int32 NaturalRow, FName ColumnId ) const;

    UFUNCTION( BlueprintCallable, Category = "Smart Tables|Pairs" )
    void TurnBack();

    UPROPERTY( BlueprintAssignable, Category = "Smart Tables|Pairs" )
    FOnSmartTablePairsTurn OnTurn;

    int32 GetMoves() const
    {
        return Moves;
    }

    int32 GetPairsFound() const
    {
        return PairsFound;
    }

    int32 GetPairsTotal() const
    {
        return PairsTotal;
    }

    bool IsComplete() const
    {
        return PairsTotal > 0 && PairsFound >= PairsTotal;
    }

protected:
    virtual int32 GetNumRows_Implementation() override;
    virtual FText GetCellText_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FLinearColor GetCellColor_Implementation( int32 NaturalRow, FName ColumnId ) override;
    virtual FName GetRowId_Implementation( int32 NaturalRow ) override;

private:
    struct FCard
    {
        int32 Symbol  = INDEX_NONE;
        bool bFaceUp  = false;
        bool bMatched = false;
    };

    int32 IndexOfColumn( FName ColumnId ) const;
    const FCard * FindCard( int32 NaturalRow, FName ColumnId ) const;
    FCard * FindCard( int32 NaturalRow, FName ColumnId );

    int32 Columns = 0;
    TArray< TArray< FCard > > Grid;

    int32 FirstRow = INDEX_NONE;
    FName FirstColumn;
    int32 SecondRow = INDEX_NONE;
    FName SecondColumn;

    int32 Moves      = 0;
    int32 PairsFound = 0;
    int32 PairsTotal = 0;
};
