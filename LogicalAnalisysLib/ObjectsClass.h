#pragma once
#include "Object.h"
#include "InformativityCondition.h"

struct ObjectsClassRange;

// ќписание положительных и отрицательных прецедентов класса
class ObjectsClass
{
public:

    int label;

    ObjectPtrList positive;

    ObjectPtrList negative;

    ObjectsClass(int l = -1);

    ObjectsClass(ObjectsClassRange const& range, int l = -1);

    UTILITY_MOVE_DEFAULT_MEMBERS(ObjectsClass, (label)(positive)(negative));

    void MakeComplement();

    void SplitOptimal(
        InformativityCondition const& informativity, 
        ObjectsClassRange& correct, 
        ObjectsClassRange& fitting,
        int posLimit);

    void SortObjectsByWeights();

    void Clear();

    void Randomize();

private:

    std::pair<int, int> DoSplitOptimal(InformativityCondition const& informativity, int posLimit);
};

typedef std::vector<ObjectsClass> ObjectsClasses;



// ќписание диапазона положительных и отрицательных прецедентов
struct ObjectsClassRange
{
    // ћетка класса
    int label;

    // ѕоложительные прецеденты
    ObjectsRange positive;

    // ќтрицательные прецеденты
    ObjectsRange negative;

    ObjectsClassRange(int l = -1):label(l){};

    ObjectsClassRange(ObjectsClass const& cls)
        :positive(cls.positive), negative(cls.negative), label(cls.label)
    {
    }
};

