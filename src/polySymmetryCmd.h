/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef POLY_SYMMETRY_CMD_H
#define POLY_SYMMETRY_CMD_H

#include "meshData.h"
#include "polySymmetry.h"

#include <vector>

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MPxToolCommand.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>

using namespace std;

#define SYMMETRY_COMPONENTS_FLAG        "-sym"
#define SYMMETRY_COMPONENTS_LONG_FLAG   "-symmetry"

#define LEFT_SIDE_VERTEX_FLAG           "-lsv"
#define LEFT_SIDE_VERTEX_LONG_FLAG      "-leftSideVertex"

#define CONSTRUCTION_HISTORY_FLAG       "-ch"
#define CONSTRUCTION_HISTORY_LONG_FLAG  "-constructionHistory"

#define EXISTS_FLAG                     "-ex"
#define EXISTS_LONG_FLAG                "-exists"


class PolySymmetryCommand : public MPxToolCommand
{
public:
                        PolySymmetryCommand();
    virtual             ~PolySymmetryCommand();

    static void*        creator();
    static MSyntax      getSyntax();

    virtual MStatus     doIt(const MArgList& argList);
    virtual MStatus     redoIt();
    virtual MStatus     undoIt();

    virtual MStatus     doQueryDataAction();
    virtual MStatus     doQueryMeshAction();
    virtual MStatus     doUndoableCommand();

    virtual MStatus     parseQueryArguments(MArgDatabase &argsData);
    virtual MStatus     parseArguments(MArgDatabase &argsData);

    virtual MStatus     getSelectedMesh(MArgDatabase &argsData);

    virtual MStatus     getSymmetryComponents(MArgDatabase &argsData);
    virtual void        setSymmetryComponents(vector<ComponentSelection> &components);

    virtual MStatus     getLeftSideVertexIndices(MArgDatabase &argsData);
    virtual void        setLeftSideVertexIndices(vector<int> &indices);

    virtual MStatus     getFlagStringArguments(MArgList &args, MSelectionList &selection);

    virtual MStatus     getSymmetricalComponentsFromNode();
    virtual MStatus     getSymmetricalComponentsFromScene();

    virtual MStatus     createResultNode();
    virtual MStatus     createResultString();

    virtual MStatus     finalize();

    const bool          isUndoable();
    const bool          hasSyntax()     { return true; } 

    static void         setJSONData(const char* key, stringstream &output, vector<int> &data, bool isLast=false);

public:
    static MString      COMMAND_NAME;

private:
    bool                        constructionHistory = false;
    bool                        isQuery = false;
    bool                        isQueryExists = false;

    MDagPath                    selectedMesh;
    MeshData                    meshData;
    PolySymmetryData            meshSymmetryData;
    
    MObject                     meshSymmetryNode;
    
    vector<ComponentSelection>  symmetryComponents;
    vector<int>                 leftSideVertexIndices;
};

#endif