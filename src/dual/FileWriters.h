#pragma once
#include "DualizationBacktrackAlgorithmBase.h"
#include "IntegersFileWriter.h"

// ����� ���� ��������
struct AllColumnsFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
    virtual void Call(DualizationNode& node);
};

// ����� ������������� ��������
class OrderedColumnsFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {

    raw_vector<int> _buffer;

public:
    virtual void Call(DualizationNode& node);
};

// ����� ������� �������
class FirstColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// ����� ���������� �������
class LastColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// ����� ����� ��������
class SizeFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// ����� ���� ��������
class WeightFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// ����� ����������� �������
class MinColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};

// ����� ����������� �������
class MaxColumnFileWriterCallback : public IDualizationCallback, public IntegersFileWriter {
public:
    virtual void Call(DualizationNode& node);
};