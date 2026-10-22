/// @file
/// @brief Base class of the backtracking dualization algorithms, search tree nodes and callbacks.

#pragma once

#include <dual/bit_matrix.h>
#include <dual/raw_vector.h>

namespace dual {

/// Set of column indices (0-based), e.g. a cover.
typedef std::vector<int> Columns;

/// @brief Scope for the search tree node types.
struct NodeType {
    /// @brief Type of a search tree node, i.e. why a callback is called.
    enum Enum {
        CoverNode,      ///< Cover node: `columns` is a minimal cover
        NewNode,        ///< New node: a column has just been added
        BacktrackNode,  ///< Previous node (backtrack): the last column has just been removed
        ExtraNode,      ///< Extra node (algorithm-specific)
        RootNode        ///< Root: the empty column set
    };
};

/// Weight of a column or of a column set.
typedef float Weight;

/// Weights indexed by column.
typedef std::vector<Weight> Weights;

/// @brief Search tree node: the current column set, passed to every `IDualizationCallback`.
///
/// Callbacks may read the node and set `stoped` (stop the whole enumeration), `pruned` (skip the
/// subtree of a new node) and `weight`.
struct DualizationNode {

    /// Columns of the current set, in the order they were added.
    Columns columns;
    /// The column added by the last `AddColumn` or removed by the last `Backtrack` (-1 if none).
    int lastColumn;
    /// Search tree node identifier: the number of nodes created since `Reset`.
    int nodeID;
    /// Search tree branch identifier: incremented when a column is added after a backtrack.
    int branchID;
    /// Node type.
    NodeType::Enum type;
    /// Current cover weight (maintained by weighting callbacks, see `WeighterBase`).
    Weight weight;
    /// Stop building the search tree; set by a callback to finish the enumeration.
    bool stoped;
    /// Prune the current branch; set by a callback for a new node to skip its subtree.
    bool pruned;
    // Restriction on the allowed columns
    //bit_chunk restriction;

    /// @brief Root node with no columns.
    DualizationNode() {
        Reset();
    }

    /// @brief Descend: append column @p j, making this a new node (and starting a new branch after
    ///        a backtrack); clears `pruned`.
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

    /// @brief Go back: remove the last column (stored in `lastColumn`).
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

    /// @brief Make this the root node: no columns, zero counters and weight, flags cleared.
    /// @param n Number of columns to reserve.
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

    /// @brief Prune the current branch if @p p (never un-prunes).
    void Prun(bool p) {
        pruned |= p;
    }

    /// @brief Check whether the node is a reported cover that was not stopped or pruned.
    bool IsCover() const {
        return !stoped && !pruned && type == NodeType::CoverNode;
    }
};

/// @brief Callback interface: algorithms call it for search tree nodes.
///
/// `DualizationBacktrackAlgorithmBase::InnerCallback` is called for every new and backtrack node,
/// `DualizationBacktrackAlgorithmBase::CoverCallback` for every found minimal cover.
struct IDualizationCallback {
    /// @brief Handle a node; may modify `node.stoped`, `node.pruned` and `node.weight`.
    /// @param node Current search tree node; valid only during the call.
    virtual void Call(DualizationNode& node) = 0;
    /// @brief Virtual destructor.
    virtual ~IDualizationCallback() = default;
};

/// @brief Adapter that turns a callable `void(DualizationNode&)` into an `IDualizationCallback`.
/// @tparam Func Callable type.
template<typename Func>
class DualizationCallbackWrap : public IDualizationCallback {
public:
    /// @brief Wrap @p func.
    explicit DualizationCallbackWrap(Func func) : func_(std::move(func)) {}

    /// @brief Call the wrapped callable.
    void Call(DualizationNode& node) override {
        func_(node);
    }

private:
    Func func_;
};

/// @brief Make a shared `IDualizationCallback` from a callable `void(DualizationNode&)`.
/// @return `std::shared_ptr<DualizationCallbackWrap<...>>`; pass `.get()` to `InnerCallback` or
///         `CoverCallback` and keep the pointer alive while the algorithm runs.
template<typename Func>
auto dualization_callback(Func&& func) {
    return std::make_shared<DualizationCallbackWrap<std::decay_t<Func>>>(std::forward<Func>(func));
}

/// Shared pointer to a callback.
typedef std::shared_ptr<IDualizationCallback> IDualizationCallbackPtr;
/// List of callbacks.
typedef std::vector<IDualizationCallbackPtr> DualizationCallbacks;

/// @brief Chains two callbacks together: calls `callback1`, then `callback2` unless the node was stopped.
/// @tparam C1 Type of the first callback (with a `Call(DualizationNode&)` method).
/// @tparam C2 Type of the second callback.
template<typename C1, typename C2>
struct ChainCallback : IDualizationCallback {
    /// First callback.
    C1 callback1;
    /// Second callback.
    C2 callback2;

    /// @brief Call `callback1`, then `callback2` if `node.stoped` is not set.
    void Call(DualizationNode& node) override {
        callback1.Call(node);
        if (node.stoped)
            return;

        callback2.Call(node);
    }
};

/// @brief Base class for a backtracking dualization algorithm.
///
/// Holds the input matrix (rows are the sets to cover, columns are the elements to choose) and the
/// callbacks through which an algorithm reports its search tree. The callbacks are not owned and
/// may be null.
class DualizationBacktrackAlgorithmBase {
protected:
    /// Current search tree node, passed to the callbacks.
    DualizationNode _currentNode;
    /// Input matrix.
    bit_matrix _matrix;
    /// Column restrictions set by `setRestrictions` (row `j`: columns related to column `j`).
    bit_matrix _restrictions;

public:
    /// Called for every new and backtrack node (not owned, may be null).
    IDualizationCallback* InnerCallback;
    /// Called for every found minimal cover (not owned, may be null).
    IDualizationCallback* CoverCallback;

    /// @brief Algorithm with an empty matrix and no callbacks.
    DualizationBacktrackAlgorithmBase();
    /// @brief Replace the input matrix; resets the current node and calls `onMatrixChanged`.
    void setMatrix(bit_matrix matrix);
    /// @brief Mutable access to the input matrix, for changing it in place.
    /// @note Counts as a matrix change (see `onMatrixChanged`; RUNC restarts its enumeration), even
    ///       if nothing is modified. Use `get_matrix()` to read the matrix.
    bit_matrix& mutable_matrix();
    /// @brief The input matrix (read-only; does not affect a running enumeration).
    bit_matrix const& get_matrix() const;
    /// @brief Load the input matrix from a hypergraph (`.hg`) file and set it with `setMatrix`.
    ///
    /// The file is text: one row per line, given by the 0-based, separated (e.g. space-separated)
    /// indices of the columns holding ones; an empty line is a zero row, trailing empty lines are
    /// ignored. The matrix has as many columns as one past the largest index. See
    /// `IntegersFileReader::ReadBitMatrix`.
    /// @throws std::runtime_error If the file cannot be opened.
    /// @throws std::out_of_range If an index does not fit into `int` (or equals `INT_MAX`).
    /// @throws std::length_error If the matrix does not fit into `int` blocks.
    void loadMatrix(std::string const& filename);
    /// @brief Set column restrictions: for every column `j`, the union of the rows of
    ///        @p restrictions that contain `j`.
    /// @note None of the live algorithms uses the restrictions.
    void setRestrictions(bit_matrix const& restrictions);
    /// @brief Load restrictions from a `.hg` file (see `loadMatrix`) and set them with `setRestrictions`.
    /// @throws std::runtime_error If the file cannot be opened.
    /// @throws std::out_of_range If an index does not fit into `int` (or equals `INT_MAX`).
    /// @throws std::length_error If the matrix does not fit into `int` blocks.
    void loadRestrictions(std::string const& filename);
    /// @brief Enumerate all minimal covers of the input matrix, reporting them to `CoverCallback`.
    virtual void dualize() = 0;
    /// @brief Virtual destructor.
    virtual ~DualizationBacktrackAlgorithmBase();

protected:
    /// @brief Call `InnerCallback` (if set) for the current node.
    void doInnerCallback();
    /// @brief Mark the current node as a cover and call `CoverCallback` (if set).
    /// @return `false` if a callback stopped the enumeration.
    bool doCoverCallback();
    /// @brief Add column @p j to the current node and call `InnerCallback`.
    /// @return `false` if a callback stopped the enumeration or pruned the new node.
    bool addColumn(int j);
    /// @brief Remove the last column from the current node and call `InnerCallback`.
    void backtrack();
    /// @brief Reset the current node to the root, reserving @p n columns.
    void resetCurrentNode(int n = 0);
    /// @brief Called after the input matrix was replaced or handed out for modification.
    virtual void onMatrixChanged() {}
};

}  // namespace dual
