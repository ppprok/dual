#pragma once
#include "DualizationBacktrackAlgorithmBase.h"
#include "IntegersFileWriter.h"

// Вывод всех столбцов
struct AllColumnsFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
    virtual void Call(DualizationNode& node);
};

// Вывод упорядоченных столбцов
class OrderedColumnsFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {

    raw_vector<int> _buffer;

public:
    virtual void Call(DualizationNode& node);
};

// Вывод первого столбца
class FirstColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Вывод последнего столбца
class LastColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Вывод числа столбцов
class SizeFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Вывод веса покрытия
class WeightFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Вывод наименьшего столбца
class MinColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Вывод наибольшего столбца
class MaxColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};