
#include "PosetsDualizationTask.h"
#include "ReversedPoset.h"
#include "bit_vector.h"

void PosetsDualizationTask::SetDefaultOptions() {
    _posetsProduct.SetDefaultOptions();
    _idealBase.Clear();
    _filterBase.Clear();
}

void PosetsDualizationTask::SetOptions(Options const& options) {
    _posetsProduct.SetOptions(options);
    LoadData(options);
}

void PosetsDualizationTask::LoadIdealBase(std::string const& filename, Options const& options) {
    _idealBase = std::move(_posetsProduct.LoadItems(filename, options));
}

void PosetsDualizationTask::LoadData(Options const& options) {
    auto filename = options.get("ideal-file-name", "");
    if (!filename.empty())
        LoadIdealBase(filename, options);
    else
        _idealBase.Clear(_posetsProduct.GetPosets().size());

    filename = options.get("filter-file-name", "");
    if (!filename.empty())
        LoadFilterBase(filename, options);
    else
        _filterBase.Clear(_posetsProduct.GetPosets().size());
}

void PosetsDualizationTask::Register() {
    Registrator<ITaskPtr>::Instance().RegisterFactory("PosetsDualizationTask",
                                                      []() { return std::make_shared<PosetsDualizationTask>(); });
}

void PosetsDualizationTask::LoadFilterBase(std::string const& filename, Options const& options) {
    _filterBase = std::move(_posetsProduct.LoadItems(filename, options));
}

void PosetsDualizationTask::ReverseTask() const {
    _posetsProduct.Reverse();
    std::swap(_filterBase, _idealBase);
}
