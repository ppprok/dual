#include "Solver.h"

namespace PosetDualization {
namespace MIGRAS {

void Solver::SetOptions(Options const& options) {
    _algorithm.SetOptions(options);

    auto f = options.get("output-file-name", "");
    if (f.empty()) {
        _algorithm.ResultCallback.Callback = 0;
    } else {
        _writer.SetFilename(f);
        _algorithm.ResultCallback.Callback = &_writer;
    }
}

void Solver::SetDefaultOptions() {
    _algorithm.SetDefaultOptions();
    _algorithm.ResultCallback.Callback = 0;
}

std::string Solver::GetResultDescription() const {
    return _algorithm.Counters.ToString();
}

void Solver::Solve(ITask* task) {
    assert(task);

    auto pdtask = dynamic_cast<PosetsDualizationTask*>(task);
    if (!pdtask)
        throw std::runtime_error("Task has wrong type");

    //_callbacksChain.use2 = pdtask->GetIdealPart() < pdtask->GetItemsCount();
    //_algorithm.Callback = &_callbacksChain;
    _algorithm.Dualize(*pdtask);
}

void Solver::Register() {
    Registrator<ISolverPtr>::Instance().RegisterFactory("MIGRASSolver", []() { return std::make_shared<Solver>(); });
}

}  // namespace MIGRAS
}  // namespace PosetDualization