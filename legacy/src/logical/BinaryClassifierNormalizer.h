#pragma once
#include "Interfaces.h"

#undef max

// Нормирование и центрирование оценок, возвращаемых классификатором
class BinaryClassifierNormalizer
    :public IBinaryClassifier, public IECVoting
{

    // Основной классификатор
    IBinaryClassifierPtr _classifier;
    
    // Сдвиг
    Estimate _shrink;
    
    // Масштаб
    Estimate _scale;

public:

    // Инициализация 
    BinaryClassifierNormalizer(IBinaryClassifierPtr classifier_);

    // Нормировка по оценкам, возвращаемым на прецедентах
    void Normalize(ObjectPtrList const& objects, Estimate from = 0, Estimate to = 1);

    // Возвращает (e + shrink) * scale, где e --- оценка внутреннего классификатора
    virtual Estimate Apply( Object const& o ) const;

    Estimate GetScale() const
    {
        return _scale;
    }

    // Увеличить масштаб
    void Scale(Estimate s)
    {
        _scale *= s;
    }

    // Увеличить сдвиг
    void Shrink(Estimate d)
    {
        _shrink += d;        
    }

    // Внутренний классификатор
    IBinaryClassifierPtr GetInner() const
    {
        return _classifier;
    }

    template<typename T>
    std::shared_ptr<T> GetInner() const
    {
        return std::dynamic_pointer_cast<T>(_classifier);
    }

    virtual void GetECSet( ECSet& ecset );
};

typedef std::shared_ptr<BinaryClassifierNormalizer> BinaryClassifierNormalizerPtr;