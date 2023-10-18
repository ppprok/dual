#pragma once
#include "DualizationBacktrackAlgorithmBase.h"
#include "IntegersFileWriter.h"

// Âûגמה גסוץ סעמכבצמג
struct AllColumnsFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
    virtual void Call(DualizationNode& node);
};

// Âûגמה ףןמנÿהמקוםםûץ סעמכבצמג
class OrderedColumnsFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {

    raw_vector<int> _buffer;

public:
    virtual void Call(DualizationNode& node);
};

// Âûגמה ןונגמדמ סעמכבצא
class FirstColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Âûגמה ןמסכוהםודמ סעמכבצא
class LastColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Âûגמה קטסכא סעמכבצמג
class SizeFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Âûגמה גוסא ןמךנûעטÿ
class WeightFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Âûגמה םאטלוםüרודמ סעמכבצא
class MinColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// Âûגמה םאטבמכüרודמ סעמכבצא
class MaxColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};