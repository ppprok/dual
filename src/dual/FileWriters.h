/// @file
/// @brief Callbacks that write the found covers (or one property of each) to a text file.

#pragma once
#include "DualizationBacktrackAlgorithmBase.h"
#include "IntegersFileWriter.h"

/// @brief Writes all columns.
///
/// As `CoverCallback`, writes one line per cover with the columns of the cover, in the order they were added,
/// separated by spaces. Open the output file with `CreateOutputFile` before the enumeration.
struct AllColumnsFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
    /// @brief Write a line for @p node if `node.IsCover()`.
    virtual void Call(DualizationNode& node);
};

/// @brief Writes the columns in sorted order.
///
/// As `CoverCallback`, writes one line per cover with the columns of the cover in increasing order, separated by
/// spaces. Open the output file with `CreateOutputFile` before the enumeration.
class OrderedColumnsFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {

    raw_vector<int> _buffer;

public:
    /// @brief Write a line for @p node if `node.IsCover()`.
    virtual void Call(DualizationNode& node);
};

/// @brief Writes the first column.
///
/// As `CoverCallback`, writes one line per cover with the first added column of the cover (nothing for the empty
/// cover). Open the output file with `CreateOutputFile` before the enumeration.
class FirstColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    /// @brief Write a line for @p node if `node.IsCover()`.
    virtual void Call(DualizationNode& node);
};

/// @brief Writes the last column.
///
/// As `CoverCallback`, writes one line per cover with the last added column of the cover (nothing for the empty
/// cover). Open the output file with `CreateOutputFile` before the enumeration.
class LastColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    /// @brief Write a line for @p node if `node.IsCover()`.
    virtual void Call(DualizationNode& node);
};

/// @brief Writes the number of columns.
///
/// As `CoverCallback`, writes one line per cover with the size of the cover. Open the output file with
/// `CreateOutputFile` before the enumeration.
class SizeFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    /// @brief Write a line for @p node if `node.IsCover()`.
    virtual void Call(DualizationNode& node);
};

/// @brief Writes the cover weight.
///
/// As `CoverCallback`, writes one line per cover with the weight of the cover with 5 digits after the point. Open the
/// output file with `CreateOutputFile` before the enumeration.
class WeightFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    /// @brief Write a line for @p node if `node.IsCover()`.
    virtual void Call(DualizationNode& node);
};

/// @brief Writes the smallest column.
///
/// As `CoverCallback`, writes one line per cover with the smallest column of the cover (nothing for the empty cover).
/// Open the output file with `CreateOutputFile` before the enumeration.
class MinColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    /// @brief Write a line for @p node if `node.IsCover()`.
    virtual void Call(DualizationNode& node);
};

/// @brief Writes the largest column.
///
/// As `CoverCallback`, writes one line per cover with the largest column of the cover (nothing for the empty cover).
/// Open the output file with `CreateOutputFile` before the enumeration.
class MaxColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    /// @brief Write a line for @p node if `node.IsCover()`.
    virtual void Call(DualizationNode& node);
};
