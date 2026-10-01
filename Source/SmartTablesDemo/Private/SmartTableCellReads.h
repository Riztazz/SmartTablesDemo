#pragma once

#include "Misc/Optional.h"
#include "SmartTableCell.h"
#include "SmartTableModel.h"
#include "SmartTableSortKey.h"

namespace SmartTableDemo
{
    inline TOptional< double > ReadNumber( const USmartTableCell & Cell )
    {
        USmartTableModel * Model = Cell.GetModel();
        if ( !Model )
        {
            return TOptional< double >();
        }

        const FSmartTableSortKey Key = Model->GetCellSortKey( Cell.GetRowIndex(), Cell.GetColumnId() );

        return Key.Kind == ESmartTableSortKeyKind::Numeric ? TOptional< double >( Key.Number ) : TOptional< double >();
    }
}
