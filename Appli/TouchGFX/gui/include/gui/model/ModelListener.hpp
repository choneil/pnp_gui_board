#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/Model.hpp>

class ModelListener
{
public:
    ModelListener() : model(0) {}
    
    virtual ~ModelListener() {}

    void bind(Model* m)
    {
        model = m;
    }

    /** Called from Model::tick when the axis positions change. */
    virtual void axisPositionsUpdated(float x, float y, float z) {}
protected:
    Model* model;
};

#endif // MODELLISTENER_HPP
