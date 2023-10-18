#pragma once

#include "Registrator.h"
#include "errors.h"
#include "string_utils.h"
#include <boost/property_tree/ptree.hpp>

typedef boost::property_tree::ptree Options;

//struct option_error: virtual error{};
//struct missing_option_error: virtual option_error{};
//struct wrong_option_error: virtual option_error{};

//////////////////////////////////////////////////////////////////////////
// Interface of customizing objects
//////////////////////////////////////////////////////////////////////////
struct IConfigurable {
    // Set options
    virtual void SetOptions(Options const& options) = 0;

    // Set default options
    virtual void SetDefaultOptions() = 0;

    // Dummy destructor
    virtual ~IConfigurable(){};
};

template<typename T>
void TrySetOptions(std::shared_ptr<T> object, Options const& options) {
    auto conf = std::dynamic_pointer_cast<IConfigurable>(object);
    if (conf)
        conf->SetOptions(options);
}

template<typename T>
void TrySetDefaultOptions(std::shared_ptr<T> object) {
    auto conf = std::dynamic_pointer_cast<IConfigurable>(object);
    if (conf)
        conf->SetDefaultOptions();
}

template<typename T>
T TryCreateChild(Options const& options, std::string const& node) {
    auto type = options.get(node + ".type", "");
    if (type.empty())
        return T();

    auto child = Registrator<T>::Instance().Create(type);
    TrySetOptions(child, options.get_child(node));
    return child;
}

/*!
    Исключение при попытке загрузить файл неизвестного формата
*/
struct file_format_error : virtual error {};

// Read options from file of json-format
Options read_json_options(std::string const& filename);

// Read options from file of info-format
Options read_info_options(std::string const& filename);

// Read options from file with auto format detection
Options read_options(std::string const& filename);

typedef std::shared_ptr<IConfigurable> IConfigurablePtr;