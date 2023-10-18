#pragma once
#include <string>

// �������� ����������� ���������� ����������
struct DualizationCounters {
    typedef long long int Int;

    Int results;       ///< ����� ��������� �������
    Int inners;        ///< ����� ���������� ���������
    Int extras;        ///< ����� ������ �����
    Int currentDelay;  ///< ������� ��������
    Int maxDelay;      ///< ������������ ��������

    // �������� ��������
    void Reset();

    // �����������, ���������� ��������
    DualizationCounters();

    // ������������� �������� � ��������� ���
    std::string ToString() const;

    void IncResult() {
        ++results;
        if (results == 1)
            maxDelay = currentDelay;
        else if (maxDelay < currentDelay)
            maxDelay = currentDelay;
        currentDelay = 0;
    }

    void IncExtra() {
        ++extras;
        ++currentDelay;
    }

    void IncInners() {
        ++inners;
        ++currentDelay;
    }
};
