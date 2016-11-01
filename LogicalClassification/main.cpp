#include "stdafx.h"

#include <boost/property_tree/info_parser.hpp>

//#include "LogicalClassificator.h"
#include "..\LogicalAnalisysLib\ECmpByRankFeatureSelector.h"
#include "..\LogicalAnalisysLib\MultiByBinaryLearner.h"
#include "..\LogicalAnalisysLib\BoostingLearner.h"
//#include "Exceptions.h"
#include "..\LogicalAnalisysLib\FTVotingLearner.h"
#include "..\LogicalAnalisysLib\AlgorithmEstimator.h"
#include "..\LogicalAnalisysLib\InitECFeatureSelector.h"
#include "..\LogicalAnalisysLib\ECVotingLearner.h"
#include "..\LogicalAnalisysLib\FTFeatureSelector.h"
#include "..\LogicalAnalisysLib\BinaryLearnerAdaptor.h"
#include "..\LogicalAnalisysLib\BinaryECFeatureSelector.h"
#include "..\LogicalAnalisysLib\LazyFeatureSelector.h"
#include "..\LogicalAnalisysLib\ClassFeatureSelector.h"

namespace po = boost::program_options;

void InitLogging();
void AddLogFile(std::string const& filename);

int main( int argc, char* argv[] )
{
    try
    {
                    
        InitLogging();
                
        po::options_description desc;    
        desc.add_options()
        ("help", "produce help message")
        ("config", po::value<std::string>(), "config file name")
        ("log", po::value<std::string>()->default_value("lc.log"), "log file name")
        ("ds", po::value<std::string>(), "dataset file name")
        ("testds", po::value<std::string>(), "test dataset file name");

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
                AddLogFile(log);

            LOG_SEV(info) << "log=" << log;
        }

        /*if (opt.count("help-algorithm"))
        {
            auto algname = opt["help-algorithm"].as<std::string>();
            auto cl = Registrator<IClassifierLearnerPtr>::Instance().TryCreate(algname);
            if (cl)
            {
                auto conf = ConfigurableCast(cl);
                if (! conf)
                {
                    std::cout << "Classifier learner "<< algname << " not configurable";
                }
                else
                {
                    std::cout << conf->GetOptionsDescription();
                }

                return 1;
            }

            auto bcl = Registrator<IBinaryClassifierLearnerPtr>::Instance().TryCreate(algname);
            if (bcl)
            {
                auto conf = ConfigurableCast(bcl);
                if (! conf)
                {
                    std::cout << "Binary classifier learner "<< algname << " not configurable";
                }
                else
                {
                    std::cout << conf->GetOptionsDescription();
                }

                return 1;
            }

            auto fs = Registrator<IFeatureSelectorPtr>::Instance().TryCreate(algname);
            if (fs)
            {
                auto conf = ConfigurableCast(fs);
                if (! conf)
                {
                    std::cout << "Feature selector "<< algname << " not configurable";
                }
                else
                {
                    std::cout << conf->GetOptionsDescription();
                }

                return 1;
            }


            std::cout << "Algorithm " << algname << " not found";
            return 2;
        }*/

        
        BoostingLearner::Register();
        MultiByBinaryLearner::Register();
        BinaryLearnerAdaptor::Register();
                
        ECmpByRankFeatureSelector::Register();
        InitECFeatureSelector::Register();
        FTFeatureSelector::Register();
        BinaryECFeatureSelector::Register();
        LazyFeatureSelector::Register();
        ClassFeatureSelector::Register();
        
        FTVotingLearner::Register();
        PSVotingLearner::Register();
        ECVotingLearner::Register();


        Options opt;

        auto config = vm["config"].as<std::string>();

        LOG_SEV(info) << "config=" << config;

        boost::property_tree::read_info(config, opt);        
        
        AlgorithmEstimator ae;
        ae.SetOptions(opt);

        if (vm.count("ds"))
            ae.trainDataset.Load(vm["ds"].as<std::string>());
        

        if (vm.count("testds"))
            ae.testDataset.Load(vm["testds"].as<std::string>());
        

        ae.Run();
    }
    catch(...)
    {
        std::cerr << boost::to_string(boost::current_exception());
        //LoggingCatch(e, logging::trivial::error);
        return -13;
    }
    
    return 0;
}

void InitLogging()
{
    //logging::core::get()->add_sink(sink);
    logging::add_console_log(std::cout, 
        keywords::filter=
        (
            expr::attr<def_severity_level>("Severity") == def_severity_level::info
        ),
        keywords::format = 
        (
            expr::stream << expr::message
        ));

    logging::add_console_log(std::clog, 
        keywords::filter=
        (
        expr::attr<def_severity_level>("Severity") >= def_severity_level::warning
        ),
        keywords::format = (
        expr::stream
        << expr::format_named_scope("Scopes", keywords::format = "%n") 
        << " [" << expr::attr<logging::trivial::severity_level>("Severity")
        << "] " << expr::message
        )
        );



}

void AddLogFile(std::string const& filename)
{
    logging::add_file_log
        (
        keywords::file_name = filename,                                        /*< file name pattern >*/
        keywords::format = (
        expr::stream
        << expr::format_named_scope("Scopes", keywords::format = "%n") 
        << " [" << expr::attr<logging::trivial::severity_level>("Severity")
        << "] " << expr::message
        )
        );
}