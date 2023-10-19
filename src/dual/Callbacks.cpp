#include "Callbacks.h"

namespace PosetDualization {

/*
CallbackResult ResultsCollector::Call( PosetsDualizationTask const& task, 
                                PosetsDualizationNode& x )
{
//    if (x.Stoped || x.Pruned)
  //      return;

    if (x.Type==PosetsDualizationNode::Result)
    {
        assert(! contains(results, x.CurrentPosetItems));
        results.push_back(x.CurrentPosetItems);
    }

    return CallbackResult::Continue;
}

void ResultsCollector::Reset()
{
    results.clear();
}*/

CallbackResult ResultsCSVFileWriter::Call(PosetsDualizationTask const& task, PosetsDualizationNode& x) {
    if (x.Type != PosetsDualizationNode::Result)  // || x.Stoped || x.Pruned)
        return CallbackResult::Continue;
    auto f = _output.get();
    assert(f);

    int n = x.CurrentPosetItems.size();
    for (int j = 0; j != n; ++j) {
        write(f, task.GetPosetsProduct().GetPosets()[j]->ToString(x.CurrentPosetItems[j]));
        if (j != n - 1)
            fputc(',', f);
    }

    fputc('\n', f);

    return CallbackResult::Continue;
}

void ResultsCSVFileWriter::SetFilename(std::string const& filename) {
    _output.reset();
    _filename.clear();

    if (!filename.empty()) {
        _output = create_file_for_write(filename);
        _filename = filename;
    }
}

void ResultsCSVFileWriter::Reset() {
    if (!_filename.empty()) {
        std::string filename = _filename;
        SetFilename(filename);
    }
}

void DecisionTreeCSVFileWriter::Reset() {
    _branch.Clear();
    ResultsCSVFileWriter::Reset();
}

CallbackResult DecisionTreeCSVFileWriter::Call(PosetsDualizationTask const& task, PosetsDualizationNode& x) {

    if (x.Type == PosetsDualizationNode::Inner)
        while (_branch.GetHeight() > 0 && !task.GetPosetsProduct().Preceq(x.CurrentPosetItems, _branch.GetBackRow()))
            _branch.DropRow();

    if (_branch.GetHeight() > 0) {
        auto f = _output.get();
        assert(f);

        auto row = _branch[_branch.GetHeight() - 1];

        switch (x.Type) {
            case PosetsDualizationNode::Result:
                write(f, "Result,");
                break;
            case PosetsDualizationNode::Inner:
                write(f, "Inner,");
                break;
            case PosetsDualizationNode::Extra:
                write(f, "Extra,");
                break;
        }

        int n = x.CurrentPosetItems.size();
        for (int j = 0; j != n; ++j) {
            write(f, task.GetPosetsProduct().GetPosets()[j]->ToString(row[j]));
            fputc(',', f);
        }
        for (int j = 0; j != n; ++j) {
            write(f, task.GetPosetsProduct().GetPosets()[j]->ToString(x.CurrentPosetItems[j]));
            if (j != n - 1)
                fputc(',', f);
        }

        fputc('\n', f);
    }

    _branch.AddRow(x.CurrentPosetItems);

    return CallbackResult::Continue;
}

}  // namespace PosetDualization