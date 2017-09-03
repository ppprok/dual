#pragma once


#include "Configurable.h"
#include "IPoset.h"
#include "Registrator.h"
#include "ISolver.h"
#include "PosetsProduct.h"



/*!
    Dualization task over posets
*/
class PosetsDualizationTask
    :public IConfigurable, public ITask
{

    mutable PosetsProduct _posetsProduct;
            
    mutable PosetItemsMatrix _idealBase; ///< Items for ideal generation grouped by columns

    mutable PosetItemsMatrix _filterBase; ///< Items for filter generation grouped by columns
    
public:
        
    /// Register factory of this type by name 'PosetsDualizationTask'
    static void Register();

    /// Get product of posets
    PosetsProduct const& GetPosetsProduct() const
    {
        return _posetsProduct;
    }
        
    /// Get product of posets 
    PosetsProduct& GetPosetsProduct()
    {
        return _posetsProduct;
    }

    /// Get items of ideal base grouped by columns
    PosetItemsMatrix const& GetIdealBase() const
    {
        return _idealBase;
    }

    /// Get items of ideal base grouped by columns
    PosetItemsMatrix const& GetFilterBase() const
    {
        return _filterBase;
    }

    /// Get items of ideal base grouped by columns
    PosetItemsMatrix& GetIdealBase()
    {
        return _idealBase;
    }

    /// Get items of ideal base grouped by columns
    PosetItemsMatrix& GetFilterBase()
    {
        return _filterBase;
    }

    // Load data for ideal and/or filter from CSV files.
    // Parameter 'options' contains file names and CSV import settings.
    void LoadData(Options const& options);

    // Load ideal base from CSV file.
    // Parameter 'options' contains CSV import settings.
    void LoadIdealBase(std::string const& filename, Options const& options);

    // Load filter base from CSV file.
    // Parameter 'options' contains CSV import settings.
    void LoadFilterBase(std::string const& filename, Options const& options);
   
    // Configure task
    virtual void SetOptions( Options const& options );
        
    // Reset all posets
    virtual void SetDefaultOptions();
    
    // Reverse posets and swap ideal and filter
    void ReverseTask() const;        
    
    
};

