#pragma once
#include "ISolver.h"
#include "PosetsProduct.h"

class FrequentInfrequentTask : public IConfigurable, public ITask {

    PosetsProduct _posetsProduct;

    PosetItemsMatrix _database;  ///< Columns of database

    typedef std::vector<int> Thresholds;

    Thresholds _thresholds;

public:
    /// Register factory of this type by name 'PosetsDualizationTask'
    static void Register();

    /*!
            ������ ������� ������������� �������
        */
    Thresholds const& GetThresholds() const {
        return _thresholds;
    }

    /*!
            ������ ������� ������������� �������
        */
    Thresholds& GetThresholds() {
        return _thresholds;
    }

    /*!
            ������������ ��������� ��������
        */
    PosetsProduct const& GetPosetsProduct() const {
        return _posetsProduct;
    }

    /*!
            ������������ ��������� ��������
        */
    PosetsProduct& GetPosetsProduct() {
        return _posetsProduct;
    }

    /*!
            ������� ���� ������
        */
    PosetItemsMatrix const& GetDatabase() const {
        return _database;
    }

    /*!
            ������� ���� ������
        */
    PosetItemsMatrix& GetDatabase() {
        return _database;
    }

    // Load database from CSV files.
    // Parameter 'options' contains file names and CSV import settings.
    void LoadDatabase(Options const& options);

    // Load database from CSV file.
    // Parameter 'options' contains CSV import settings.
    void LoadDatabase(std::string const& filename, Options const& options);

    // Configure task
    virtual void SetOptions(Options const& options);

    // Reset all posets
    virtual void SetDefaultOptions();
};
