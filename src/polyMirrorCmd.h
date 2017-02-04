/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef POLY_MIRROR_COMMAND_H
#define POLY_MIRROR_COMMAND_H

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MPxCommand.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>

class PolyMirrorCommand : public MPxCommand
{
public:
                        PolyMirrorCommand();
    virtual             ~PolyMirrorCommand();

    static void*        creator();
    static MSyntax      getSyntax();

    virtual MStatus     doIt(const MArgList& argList);
    virtual MStatus     redoIt();
    virtual MStatus     undoIt();

    virtual bool        isUndoable() const { return true; }
    virtual bool        hasSyntax()  const { return true; }

public:
    static MString      COMMAND_NAME;

private:    
    MPointArray         originalPoints;
    MObject             polySymmetryData;

    MDagPath            baseMesh;
    MDagPath            targetMesh;
};
#endif 
