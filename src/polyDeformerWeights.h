/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef POLY_DEFORMER_WEIGHTS_H
#define POLY_DEFORMER_WEIGHTS_H

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MFloatArray.h>
#include <maya/MObject.h>
#include <maya/MPxCommand.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>

class PolyDeformerWeightsCommand : public MPxCommand
{
public:
                        PolyDeformerWeightsCommand();
    virtual             ~PolyDeformerWeightsCommand();

    static void*        creator();
    static MSyntax      getSyntax();

    virtual MStatus     parseArguments(MArgDatabase &argsData);
    virtual MStatus     validateArguments();

    virtual MStatus     getNodeArgument(MArgDatabase &argsData, const char* flag, MObject &node, bool required);
    virtual MStatus     getDagPathArgument(MArgDatabase &argsData, const char* flag, MDagPath &path, bool required);

    virtual MStatus     doIt(const MArgList& argList);
    virtual MStatus     redoIt();
    virtual MStatus     undoIt();

    virtual void        getAllComponents(MDagPath &mesh, MObject &components);
    
    virtual bool        isUndoable() const { return true; }
    virtual bool        hasSyntax()  const { return true; }

public:
    static MString      COMMAND_NAME;

private:    
    int                 direction     = 1;
    bool                mirrorWeights = false;
    bool                flipWeights   = false;
    
    uint                geometryIndex;

    MFloatArray         oldWeightValues;

    MDagPath            sourceMesh;
    MDagPath            destinationMesh;

    MObject             polySymmetryData;

    MObject             sourceDeformer;
    MObject             destinationDeformer;
    MObject             components;
};

#endif 