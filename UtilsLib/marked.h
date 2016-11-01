#pragma once

// Метка к объекту базового типа
template<typename base>
struct marked : public base
{
    static const int no_value = INT_MIN;	

    int _value;

public:

    
    marked(base b = base(), int value = no_value)
        : base(b), _value(value)          
    {
    }

    bool has_value() const
    {
        return _value != no_value;
    }

    void reset_value(int value = no_value)
    {
        _value = value;
    }

    void set_value(int value)
    {
        _value = value;
    }

    int get_value() const
    {
        return _value;
    }

};
