#pragma once

#include <map>
#include <boost/function.hpp>

#include "errors.h"

// ����������� ���������� �������� � ������ �������� ���������� ���� �� ����� 
template <typename T>
class Registrator
{
    typedef std::map<std::string, T> InstanceMap;

    typedef boost::function<T()> Factory;

    typedef std::map<std::string, Factory> FactoryMap;

    InstanceMap _instances;

    FactoryMap _factories;

public:

    void Register(std::string const& name, T const& instance)
    {
        _instances[name] = instance;
    }

    template <typename F>
    void RegisterFactory(std::string const& name, F factory)
    {
        _factories[name] = factory;
    }

    T TryCreate(std::string const& name)
    {
        auto pos = _factories.find(name);
        if (pos == _factories.end())
            return T();
        return pos->second();
    }

    T& TryGet(std::string const& name)
    {
        auto pos = _instances.find(name);
        if (pos == _instances.end())
        {
            static T null;
            return null;
        }
        return pos->second;
    }

    T& Get(std::string const& name)
    {
        auto pos = _instances.find(name);
        if (pos == _instances.end())
            BOOST_THROW_EXCEPTION(error() << message("Object isn't registered") << object_name(name));
        return pos->second;
    }

    T Create(std::string const& name)
    {
        auto pos = _factories.find(name);
        if (pos == _factories.end())
            BOOST_THROW_EXCEPTION(error() << message("Factory isn't registered") << object_name(name));
        return pos->second();
    }


    static Registrator& Instance()
    {
        static Registrator instance;
        return instance;
    }
};