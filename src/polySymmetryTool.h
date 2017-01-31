/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef POLY_SYMMETRY_TOOL_H
#define POLY_SYMMETRY_TOOL_H

#include "meshData.h"
#include "polySymmetry.h"

#include <vector>

#include <maya/MColor.h>
#include <maya/MColorArray.h>
#include <maya/MDagPath.h>
#include <maya/MEvent.h>
#include <maya/MPxContext.h>
#include <maya/MPxContextCommand.h>
#include <maya/MPxSelectionContext.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MStatus.h>

using namespace std;

const int VERTICES = 1;
const int COMPONENTS = 2;

class PolySymmetryTool : public MPxSelectionContext
{
public:
                        PolySymmetryTool();
    virtual             ~PolySymmetryTool();

    virtual void        toolOnSetup(MEvent &event);
    virtual void        toolOffCleanup();

    virtual MStatus     helpStateHasChanged(MEvent &event);

    virtual void        abortAction();
    virtual void        completeAction();
    virtual void        deleteAction();

    virtual void        doToolCommand();

    virtual MStatus     getSelectedMesh();
    virtual MStatus     clearSelectedMesh();

    virtual void        updateHelpString();
    virtual void        recalculateSymmetry();
    virtual void        updateDisplayColors();

private:
    MDagPath                    selectedMesh;    
    MeshData                    meshData;
    PolySymmetryData            symmetryData;

    MColorArray                 originalVertexColors;
    bool                        originalDisplayColors;
    bool                        hasVertexColors;

    vector<int>                 affectedVertices;

    vector<ComponentSelection>  selectedComponents;
    vector<int>                 leftSideVertexIndices;
};

class PolySymmetryContextCmd : public MPxContextCommand
{
public:
    PolySymmetryContextCmd();
    ~PolySymmetryContextCmd();

    virtual MPxContext*     makeObj();
    static void*            creator();

public:
    static MString          COMMAND_NAME;
};

#endif