#pragma once

#include "IPoset.h"
#include "PosetsDualizationTask.h"
#include "PosetsDualizationNode.h"
#include "PosetItemsMatrix.h"

namespace PosetDualization
{

    enum CallbackResult
    {
        Stop,
        Continue,
        Prun,
        WasDelayed
    };

    // Интерфейс обратного вызова
    struct ICallback
    {
        virtual CallbackResult Call(
            PosetsDualizationTask const& task, 
            PosetsDualizationNode& current) = 0;

        virtual void Reset() = 0;

        virtual ~ICallback(){};
    };

    /*
    template<typename C1, typename C2>
    struct CallbacksPair:
        ICallback
    {
        bool use1;
        C1 callback1;

        bool use2;
        C2 callback2;

        CallbacksPair()
            :use1(true), use2(true)
        {
        }

        virtual CallbackResult Call(PosetsDualizationTask const& task, 
            PosetsDualizationNode& current)
        {
            if (use1)
            {
                callback1.Call(task, current);
                if (current.Stoped)
                    return;
            }
            if (use2)
                callback2.Call(task, current);
        }

        virtual void Reset()
        {
            if (use1)
                callback1.Reset();
            if (use2)
                callback2.Reset();
        }
    };

    template<typename C1, typename C2, typename C3>
    struct CallbacksTriple:
        ICallback
    {
        bool use1;
        C1 callback1;

        bool use2;
        C2 callback2;

        bool use3;
        C3 callback3;

        CallbacksTriple()
            :use1(true), use2(true), use3(true)
        {
        }

        virtual CallbackResult Call(PosetsDualizationTask const& task, 
            PosetsDualizationNode& current)
        {
            if (use1)
            {
                callback1.Call(task, current);
                if (current.Stoped)
                    return;
            }
            if (use2)
            {
                callback2.Call(task, current);
                if (current.Stoped)
                    return;
            }

            if (use3)
                callback3.Call(task, current);
        }

        virtual void Reset()
        {
            if (use1)
                callback1.Reset();
            if (use2)
                callback2.Reset();
            if (use3)
                callback3.Reset();
        }

        ICallback* GetCallback()
        {
            if (use1 && ! use2 && ! use3)
                return &callback1;
            if (! use1 && use2 && ! use3)
                return &callback2;
            if (! use1 && ! use2 && use3)
                return &callback3;
            if (! use1 && ! use2 && ! use3)
                return 0;
            return this;
        }
    };*/


    class ResultsCSVFileWriter
        :public ICallback
    {
    protected:    

        FILE_ptr _output;
        std::string _filename;

    public:

        void SetFilename(std::string const& filename);

        std::string const& GetFilename() const
        {
            return _filename;
        }

        void Reset();

        virtual CallbackResult Call( 
            PosetsDualizationTask const& task, 
            PosetsDualizationNode& x );
    };

    class DecisionTreeCSVFileWriter
        :public ResultsCSVFileWriter
    {
        PosetItemsMatrix _branch;   

    public:

        virtual CallbackResult Call( 
            PosetsDualizationTask const& task, 
            PosetsDualizationNode& x );

        virtual void Reset();
    };



    /*!
    * Callback for dualization algorithm over posets
    * that collect all results in memory
    
    class ResultsCollector
        :public ICallback
    {

    public:

        PosetItemsMatrix results;

        virtual CallbackResult Call( PosetsDualizationTask const& task, 
            PosetsDualizationNode& x );

        virtual void Reset();
    };*/

}