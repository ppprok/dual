#pragma once
#include "DualizationBacktrackAlgorithmBase.h"
#include "IntegersFileWriter.h"

// Writes all columns
struct AllColumnsFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
    virtual void Call(DualizationNode& node);
};

// Writes the columns in sorted order
class OrderedColumnsFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {

    raw_vector<int> _buffer;

public:
    virtual void Call(DualizationNode& node);
};

// Writes the first column
class FirstColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Writes the last column
class LastColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Writes the number of columns
class SizeFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Writes the cover weight
class WeightFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Writes the smallest column
class MinColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Writes the largest column
class MaxColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};