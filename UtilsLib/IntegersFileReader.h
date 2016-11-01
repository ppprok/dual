#pragma once
#include <stdio.h>
#include <io.h>
#include <boost/noncopyable.hpp>

#include "bit_matrix.h"
#include "io_utils.h"

// Чтение из файла целочисленных данных
class IntegersFileReader
	:boost::noncopyable
{
    FILE_ptr _input;
    
	bit_matrix _bm;
    
	int _lastChar;

private:

    bool IsDigitChar(int c) const
	{
		return '0' <= c && c <= '9';
	}

    int ReadUnsignedInteger(int firstDigit);

	int EstimateBitMatrixCapacity() const;

public:
		
	IntegersFileReader();
    	
	// Конструктор с открытием файла
	IntegersFileReader(std::string const& filename);
		
	// Открыть файл для чтения
    void OpenInputFile(std::string const& filename);
    
	// Закрыть файл
	void CloseInputFile();

	// Проверка конца файла
	bool IsEOF() const;
		
    // Прочитать битовую матрицу из файла
	bit_matrix ReadBitMatrix();

    // Прочитать битовую матрицу из файла
    std::vector<bits32> ReadBitMatrix32(int first = 0);

    // Прочитать битовую матрицу из файла
    std::vector<bits64> ReadBitMatrix64(int first = 0);

	// Прочитать целое число из файла
	int ReadUnsignedInteger();

	// Прочитать из файла набор целых чисел до перехода на следующую строку
	template <typename Container>
	void ReadUnsignedIntegers(Container& integers, char delim = '\n')
	{
		assert(_input);
		while(! feof(_input.get()))
		{
			_lastChar = getc(_input.get());
			if (IsDigitChar(_lastChar))
				integers.push_back(ReadUnsignedInteger(_lastChar - '0'));

			if (_lastChar == delim)
				return;
		}
	}
	    
	// Прочитать из файла матрицу (список списков) целых чисел, разделенных символом '\n'
	template <typename Container>
	void ReadUnsignedIntegersMatrix(Container& integersList)
	{
		assert(_input);
		while(! feof(_input.get()))
		{
			integersList.resize(integersList.size()+1);
			ReadUnsignedIntegers(integersList.back());
		}

        while(! integersList.empty() &&
            integersList.back().empty())
            integersList.pop_back();
	}

	
};