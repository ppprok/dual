// Dualizer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "logging_utils.h"
#include "Configurable.h"
#include "Registrator.h"
#include "ISolver.h"
#include "MIGRAS.h"
#include "Solver.h"
#include "PosetsDualizationTask.h"
//#include "Antichain.h"
#include "GraphPoset.h"
#include "NumberChain.h"
#include "IntervalLattice.h"




int main(int argc, char* argv[])
{
    try
    {
        add_clog_log();
        add_cout_log();
            
        po::options_description desc;    
        desc.add_options()
            ("help", "produce help message")
            ("solver", po::value<std::string>(), "solver config file name")
            ("task", po::value<std::string>(), "task config file name")
            ("input", po::value<std::string>(), "input file name")
            ("output", po::value<std::string>(), "output file name")
            ("log", po::value<std::string>()->default_value("dualizer.log"), "log file name");

        po::positional_options_description p;
        p.add("config", -1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
        po::notify(vm);

        if (vm.count("help"))
        {
            std::cout << desc;
            return 1;
        }

        if (vm.count("log"))
        {
            auto log = vm["log"].as<std::string>();
            if (! log.empty())
                add_log_file(log);

            LOG_SEV(info) << "log=" << log;
        }

        PosetDualization::MIGRAS::Solver::Register();
        PosetsDualizationTask::Register();
        
        //Antichain::Register();
        GraphPoset::Register();
        NumberChain::Register();
        IntervalLattice::Register();
        
        if (! vm.count("solver"))
        {
            LOG_SEV(error) << "Solver config file name wasn't defined (--solver option)";
            return -1;
        }        

        if (! vm.count("task"))
        {
            LOG_SEV(error) << "Task config file name wasn't defined (--task option)";
            return -1;
        }

        auto solverOptions = read_options(vm["solver"].as<std::string>());
        auto solver = Registrator<ISolverPtr>::Instance().Create(solverOptions.get<std::string>("type"));        
        assert(solver);
        if (vm.count("output"))
            solverOptions.add("output-file-name", vm["output"].as<std::string>());        
        TrySetOptions(solver, solverOptions);

        
        auto taskOptions = read_options(vm["task"].as<std::string>());
        auto task = Registrator<ITaskPtr>::Instance().Create(taskOptions.get<std::string>("type"));
        assert(task);
        if (vm.count("input"))
            taskOptions.add("data-file-name", vm["input"].as<std::string>());
        TrySetOptions(task, taskOptions);
        
        solver->Solve(task.get());
        LOG_SEV(info) << solver->GetResultDescription();

    }
    catch(...)
    {
        std::cout << "View log for error information";
        log_catched();
        return -2;
    }
    
    return 0;
}


