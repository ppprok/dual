#pragma once

#include <cassert>
#include <charconv>
#include <list>
#include <string>
#include <vector>

#include "blob.h"
#include "chunk.h"
#include "containers.h"
#include "errors.h"
#include "io_utils.h"

typedef chunk<char> value_chunk;

typedef std::vector<value_chunk> value_chunks;

// Column data
struct CSVArray {
    //value_chunk name;
    value_chunks values;

    std::string GetName() const {
        if (values.empty())
            return "???";

        return GetValue(0);
    }

    std::string GetValue(value_chunk const& v) const {
        if (!v.valid())
            return "";

        return std::string(v.begin, v.end);
    }

    // Get i-th value string
    std::string GetValue(int i) const {
        assert(inside(values, i));
        return GetValue(values[i]);
    }

    // Get i-th value of type T
    template<typename T>
    T GetValue(int i) const {
        auto s = GetValue(i);
        T val;
        std::from_chars(s.data(), s.data() + s.size(), val);
        return val;
    }

    // Get i-th value of type T
    template<typename T>
    T GetValue(value_chunk const& v) const {
        auto s = GetValue(v);
        return std::from_chars<T>(s.data(), s.data() + s.size());
    }

    void AddValueChar(char* ref) {
        assert(ref);
        if (values.empty() || values.back().end != ref)
            values.emplace_back(ref, ref + 1);
        else
            ++values.back().end;
    }

    template<typename IsSpace>
    void TrimSpaces(value_chunk& value, IsSpace isSpace) {
        for (; value.begin != value.end; ++value.begin)
            if (!isSpace(*value.begin))
                break;

        for (; value.begin != value.end; --value.end)
            if (!isSpace(*(value.end - 1)))
                break;
    }

    template<typename IsSpace>
    void TrimSpaces(IsSpace isSpace) {
        //TrimSpaces(name, isSpace);
        for (auto& v : values)
            TrimSpaces(v, isSpace);
    }
};

typedef std::vector<CSVArray> CSVArrays;

struct CharMap {
    char selected[256];

    CharMap(std::string const& select = "") {
        memset(selected, 0, 256);
        for (auto c : select)
            selected[c] = 1;
    }

    template<typename Selector>
    CharMap(Selector const& selector) {
        for (int c = 0; c != 256; ++c)
            selected[c] = selector((char) c) ? 1 : 0;
    }

    bool operator()(char c) const {
        return selected[c] != 0;
    }
};

//////////////////////////////////////////////////////////////////////////
// Load comma separated values from file
//////////////////////////////////////////////////////////////////////////
class CSVLoader {
    typedef blob<char> text_blob;

    typedef std::list<text_blob> text_blobs;

    text_blob _data;

    text_blobs _appendix;

    CSVArrays _arrays;

public:
    // Get colums
    CSVArrays const& GetColumns() const {
        return _arrays;
    }

    int GetColumnsHeight() const {
        return _arrays.empty() ? 0 : _arrays.front().values.size();
    }

    // Load all text data from file
    void Load(std::string const& filename) {
        SetData(read_all(filename));
    }

    // Load all text data from file and append to
    // before loaded data
    void Append(std::string const& filename) {
        AddData(read_all(filename));
    }

    // Explicitly set CSV data
    void SetData(std::string data) {
        _data.assign_range(data.begin(), data.end());
        _arrays.clear();
        _appendix.clear();
    }

    // Append CSV data chunk
    void AddData(std::string data) {
        if (_data.empty() || _appendix.empty())
            SetData(std::move(data));
        else
            _appendix.emplace_back(data.begin(), data.end());
    }

    template<typename IsSeparator, typename IsEOL>
    void CreateColumns(IsSeparator const& isSeparator,
                       IsEOL const& isEOL,
                       bool namesInFirstRow = true,
                       bool skipFirstRow = false) {
        _arrays.clear();
        CreateColumns(_data, isSeparator, isEOL, skipFirstRow);

        if (namesInFirstRow)
            skipFirstRow = true;

        for (auto& data : _appendix)
            CreateColumns(data, isSeparator, isEOL, skipFirstRow);
    }

    template<typename IsSeparator, typename IsEOL>
    void CreateRows(IsSeparator const& isSeparator, IsEOL const& isEOL) {
        _arrays.clear();
        CreateRows(_data, isSeparator, isEOL);
        for (auto& data : _appendix)
            CreateRows(data, isSeparator, isEOL);
    }

    template<typename IsSpace>
    void TrimSpaces(IsSpace const& isSpace) {
        for (auto& column : _arrays)
            column.TrimSpaces(isSpace);
    }

    void TrimSpaces() {
        TrimSpaces([](char c) { return c == ' ' || c == '\t' || c == '\r'; });
    }

    // Create default columns structures
    void CreateColumns();

    // Create default rows structures
    void CreateRows();

    CSVArray& GetColumn(std::string const& name) {
        for (auto& column : _arrays) {
            if (column.values.empty())
                continue;
            if (column.GetValue(0) == name)
                return column;
        }

        throw std::runtime_error("Column '" + name + "' doesn't exist");
    }

    CSVArray const& GetColumn(std::string const& name) const {
        return GetColumn(name);
    }

public:
    // Create columns structures from loaded data
    template<typename IsSeparator, typename IsEOL>
    void CreateColumns(text_blob& data, IsSeparator const& isSeparator, IsEOL const& isEOL, bool skipFirstRow = false) {
        int column = 0;
        bool eol = true;
        int row = -1;

        for (char *ref = data.begin(), *e = data.end(); ref != e; ++ref) {
            char c = *ref;
            if (isEOL(c)) {
                eol = true;
                continue;
            }

            if (eol) {
                eol = false;
                column = 0;
                ++row;
            }

            if (isSeparator(c)) {
                ++column;
                continue;
            }

            if ((int) _arrays.size() <= column)
                _arrays.resize(column + 1);

            if (row == 0 && skipFirstRow)
                continue;

            _arrays[column].AddValueChar(ref);
        }
    }

    // Create columns structures from loaded data
    template<typename IsSeparator, typename IsEOL>
    void CreateRows(text_blob& data, IsSeparator const& isSeparator, IsEOL const& isEOL) {
        bool eol = true;

        for (char *ref = data.begin(), *e = data.end(); ref != e; ++ref) {
            char c = *ref;
            if (isEOL(c)) {
                eol = true;
                continue;
            }

            if (eol) {
                if (_arrays.empty() || !_arrays.back().values.empty())
                    _arrays.emplace_back();
                eol = false;
            }

            if (isSeparator(c))
                continue;

            _arrays.back().AddValueChar(ref);
        }

        if (!_arrays.empty() && _arrays.back().values.empty())
            _arrays.pop_back();
    }
};