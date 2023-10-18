#pragma once

#include "bit_matrix.h"
#include "raw_vector.h"

typedef std::vector<int> Columns;

struct NodeType {
    enum Enum {
        CoverNode,      // Узел покрытия
        NewNode,        // Новый узел
        BacktrackNode,  // Предыдущий узел
        ExtraNode,      // Лишний узел
        RootNode        // Корень
    };
};

typedef float Weight;

typedef std::vector<Weight> Weights;

// Узел дерева решений
struct DualizationNode {

    // Столбцы
    Columns columns;
    // Вес набора столбцов
    int lastColumn;
    // Идентификатор узла ДР
    int nodeID;
    // Идентификатор ветви ДР
    int branchID;
    // Тип узла
    NodeType::Enum type;
    // Текущий вес покрытия
    Weight weight;
    // Остановить построение ДР
    bool stoped;
    // Отсечь текущую ветку
    bool pruned;
    // Ограничение на допустимые столбцы
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

// Интерфейс обратного вызова
struct IDualizationCallback {
    virtual void Call(DualizationNode& node) = 0;

    virtual ~IDualizationCallback(){};
};

typedef std::shared_ptr<IDualizationCallback> IDualizationCallbackPtr;

typedef std::vector<IDualizationCallbackPtr> DualizationCallbacks;

// Склеивание двух обратных вызовов в цепочку
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

// Базовый класс для backtrack-алгоритма дуализации
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
