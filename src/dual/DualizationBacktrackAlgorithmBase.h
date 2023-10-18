#pragma once

#include "bit_matrix.h"
#include "raw_vector.h"

typedef std::vector<int> Columns;

struct NodeType {
    enum Enum {
        CoverNode,      // ���� ��������
        NewNode,        // ����� ����
        BacktrackNode,  // ���������� ����
        ExtraNode,      // ������ ����
        RootNode        // ������
    };
};

typedef float Weight;

typedef std::vector<Weight> Weights;

// ���� ������ �������
struct DualizationNode {

    // �������
    Columns columns;
    // ��� ������ ��������
    int lastColumn;
    // ������������� ���� ��
    int nodeID;
    // ������������� ����� ��
    int branchID;
    // ��� ����
    NodeType::Enum type;
    // ������� ��� ��������
    Weight weight;
    // ���������� ���������� ��
    bool stoped;
    // ������ ������� �����
    bool pruned;
    // ����������� �� ���������� �������
    //bit_chunk restriction;

    DualizationNode() {
        Reset();
    }

    void AddColumn(int j) {
        columns.push_back(j);
        lastColumn = j;
        ++nodeID;
        if (type != NodeType::NewNode) {
            type = NodeType::NewNode;
            ++branchID;
        }
        pruned = false;
        //restriction.reset();
    }

    void Backtrack() {
        if (!columns.empty()) {
            lastColumn = columns.back();
            columns.pop_back();
        } else {
            lastColumn = -1;
        }
        type = NodeType::BacktrackNode;
        //restriction.reset();
    }

    void Reset(int n = 0) {
        columns.clear();
        if (n > 0)
            columns.reserve(n);
        type = NodeType::RootNode;
        lastColumn = -1;
        nodeID = 0;
        branchID = 0;
        weight = 0;
        stoped = false;
        pruned = false;
        //restriction.reset();
    }

    void Prun(bool p) {
        pruned |= p;
    }

    bool IsCover() const {
        return !stoped && !pruned && type == NodeType::CoverNode;
    }
};

// ��������� ��������� ������
struct IDualizationCallback {
    virtual void Call(DualizationNode& node) = 0;

    virtual ~IDualizationCallback(){};
};

typedef std::shared_ptr<IDualizationCallback> IDualizationCallbackPtr;

typedef std::vector<IDualizationCallbackPtr> DualizationCallbacks;

// ���������� ���� �������� ������� � �������
template<typename C1, typename C2>
struct ChainCallback : IDualizationCallback {
    C1 callback1;
    C2 callback2;

    virtual void Call(DualizationNode& node) {
        callback1.Call(node);
        if (node.stoped)
            return;

        callback2.Call(node);
    }
};

// ������� ����� ��� backtrack-��������� ����������
class DualizationBacktrackAlgorithmBase {
protected:
    DualizationNode _currentNode;

    bit_matrix _matrix;

    bit_matrix _restrictions;

public:
    DualizationBacktrackAlgorithmBase();

    IDualizationCallback* InnerCallback;

    IDualizationCallback* CoverCallback;

    void SetMatrix(bit_matrix matrix);

    void LoadMatrix(std::string const& filename);

    void SetRestrictions(bit_matrix const& restrictions);

    void LoadRestrictions(std::string const& filename);

    virtual void Dualize() = 0;

    virtual ~DualizationBacktrackAlgorithmBase(){};

protected:
    void DoInnerCallback();

    bool DoCoverCallback();

    bool AddColumn(int j);

    void Backtrack();

    void ResetCurrentNode(int n = 0);
};
