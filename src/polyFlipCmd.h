/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef POLY_FLIP_CMD_H
#define POLY_FLIP_CMD_H

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MPxCommand.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>

using namespace std;

class PolyFlipCommand : public MPxCommand
{
public:
                        PolyFlipCommand();
    virtual             ~PolyFlipCommand();

    static void*        creator();
    static MSyntax      getSyntax();

    virtual MStatus     doIt(const MArgList& argList);
    virtual MStatus     redoIt();
    virtual MStatus     undoIt();

    virtual MStatus     flipMesh();
    virtual MStatus     flipMeshAgainst();

    virtual bool        isUndoable() const { return true; }
    virtual bool        hasSyntax()  const { return true; }

public:
    static MString      COMMAND_NAME;

private:    
    bool                worldSpace = false;
    bool                objectSpace = true;

    MPointArray         originalPoints;
    MObject             polySymmetryData;
    MDagPath            selectedMesh;
    MDagPath            referenceMesh;
};

#endif