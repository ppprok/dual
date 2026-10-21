#pragma once

#include "bit_matrix.h"
#include "raw_vector.h"

typedef std::vector<int> Columns;

struct NodeType {
    enum Enum {
        CoverNode,      // Cover node
        NewNode,        // New node
        BacktrackNode,  // Previous node (backtrack)
        ExtraNode,      // Extra node
        RootNode        // Root
    };
};

typedef float Weight;

typedef std::vector<Weight> Weights;

// Search tree node
struct DualizationNode {

    // Columns
    Columns columns;
    // Weight of the column set
    int lastColumn;
    // Search tree node identifier
    int nodeID;
    // Search tree branch identifier
    int branchID;
    // Node type
    NodeType::Enum type;
    // Current cover weight
    Weight weight;
    // Stop building the search tree
    bool stoped;
    // Prune the current branch
    bool pruned;
    // Restriction on the allowed columns
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

// Callback interface
struct IDualizationCallback {
    virtual void Call(DualizationNode& node) = 0;
    virtual ~IDualizationCallback() = default;
};

template<typename Func>
class DualizationCallbackWrap : public IDualizationCallback {
public:
    explicit DualizationCallbackWrap(Func func) : func_(std::move(func)) {}

    void Call(DualizationNode& node) override {
        func_(node);
    }

private:
    Func func_;
};

template<typename Func>
auto dualization_callback(Func&& func) {
    return std::make_shared<DualizationCallbackWrap<std::decay_t<Func>>>(std::forward<Func>(func));
}

typedef std::shared_ptr<IDualizationCallback> IDualizationCallbackPtr;
typedef std::vector<IDualizationCallbackPtr> DualizationCallbacks;

// Chains two callbacks together
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

// Base class for a backtracking dualization algorithm
class DualizationBacktrackAlgorithmBase {
protected:
    DualizationNode _currentNode;
    bit_matrix _matrix;
    bit_matrix _restrictions;

public:
    IDualizationCallback* InnerCallback;
    IDualizationCallback* CoverCallback;

    DualizationBacktrackAlgorithmBase();
    void setMatrix(bit_matrix matrix);
    // Mutable access counts as a matrix change (see onMatrixChanged)
    bit_matrix& get_matrix();
    bit_matrix const& get_matrix() const;
    void loadMatrix(std::string const& filename);
    void setRestrictions(bit_matrix const& restrictions);
    void loadRestrictions(std::string const& filename);
    virtual void dualize() = 0;
    virtual ~DualizationBacktrackAlgorithmBase();

protected:
    void doInnerCallback();
    bool doCoverCallback();
    bool addColumn(int j);
    void backtrack();
    void resetCurrentNode(int n = 0);
    // Called after the input matrix was replaced or handed out for modification
    virtual void onMatrixChanged() {}
};
