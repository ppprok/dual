#include "stdafx.h"
#include "FrequentInfrequentTask.h"
#include "Registrator.h"


void FrequentInfrequentTask::Register()
{
    Registrator<ITaskPtr>::Instance().RegisterFactory("FrequentItemsTask", 
        [](){return std::make_shared<FrequentInfrequentTask>();});
}

void FrequentInfrequentTask::LoadDatabase( Options const& options )
{
    auto filename = options.get("file-name", "");
    if (! filename.empty())
        LoadDatabase(filename, options);
    else
        _database.Clear(_posetsProduct.GetPosets().size());

}

void FrequentInfrequentTask::LoadDatabase( std::string const& filename, Options const& options )
{
    _database = std::move(_posetsProduct.LoadItems(filename, options));
}

void FrequentInfrequentTask::SetOptions( Options const& options )
{
    _posetsProduct.SetOptions(options);

    _thresholds.clear();
    for(auto& p : options.get_child("thresholds"))
        _thresholds.push_back(p.second.get_value<int>());
    
    LoadDatabase(options);
}

void FrequentInfrequentTask::SetDefaultOptions()
{
    _posetsProduct.SetDefaultOptions();
    _database.Clear();
    _thresholds.clear();
}