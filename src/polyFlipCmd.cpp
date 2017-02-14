/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include <vector>

#include "polyFlipCmd.h"
#include "polySymmetryNode.h"
#include "sceneCache.h"

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MItGeometry.h>
#include <maya/MIntArray.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MPxCommand.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>

using namespace std;

#define WORLD_SPACE_FLAG "-ws"
#define WORLD_SPACE_LONG_FLAG "-worldSpace"

#define OBJECT_SPACE_FLAG "-os"
#define OBJECT_SPACE_LONG_FLAG "-objectSpace"

#define REFERENCE_MESH_FLAG "-ref"
#define REFERENCE_MESH_LONG_FLAG "-referenceFlag"

PolyFlipCommand::PolyFlipCommand()  {}
PolyFlipCommand::~PolyFlipCommand() {}

void* PolyFlipCommand::creator()
{
    return new PolyFlipCommand();
}

MSyntax PolyFlipCommand::getSyntax()
{
    MSyntax syntax;

    syntax.useSelectionAsDefault(true);
    syntax.setObjectType(MSyntax::kSelectionList, 1, 1);

    syntax.addFlag(WORLD_SPACE_FLAG, WORLD_SPACE_LONG_FLAG);
    syntax.addFlag(OBJECT_SPACE_FLAG, OBJECT_SPACE_LONG_FLAG);
    syntax.addFlag(REFERENCE_MESH_FLAG, REFERENCE_MESH_LONG_FLAG, MSyntax::kSelectionItem);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}

MStatus PolyFlipCommand::doIt(const MArgList& argList)
{
    MStatus status;

    MArgDatabase argsData(syntax(), argList);

    MSelectionList selection;
    argsData.getObjects(selection);

    status = selection.getDagPath(0, this->selectedMesh);

    if (!status || !this->selectedMesh.hasFn(MFn::kMesh))
    {
        MGlobal::displayError("polyFlip command requires a mesh.");
        return MStatus::kFailure;
    }
    
    bool cacheHit = PolySymmetryCache::getNodeFromCache(this->selectedMesh, this->polySymmetryData);

    if (!cacheHit)
    {
        MString errorMsg("^1s has not had it's symmetry computed.");
        errorMsg.format(errorMsg, selectedMesh.partialPathName());

        MGlobal::displayError(errorMsg);

        return MStatus::kFailure;
    }

    if (argsData.isFlagSet(WORLD_SPACE_FLAG))
    {
        this->worldSpace = true;
    }

    if (argsData.isFlagSet(OBJECT_SPACE_FLAG))
    {
        this->worldSpace = true;
    }

    if (argsData.isFlagSet(REFERENCE_MESH_FLAG))
    {
        MSelectionList referenceMeshSelection;

        argsData.getObjects(referenceMeshSelection);

        status = referenceMeshSelection.getDagPath(0, this->referenceMesh);
        
        if (!status || !this->referenceMesh.hasFn(MFn::kMesh))
        {
            MString errorMsg("%s flag requires a mesh.");
            errorMsg.format(errorMsg, MString(REFERENCE_MESH_LONG_FLAG));

            MGlobal::displayError(errorMsg);
            return MStatus::kFailure;
        }

        MFnMesh fnRef(this->selectedMesh);
        MFnMesh fnSel(this->referenceMesh);

        if (
            (fnRef.numVertices() != fnSel.numVertices())
            || (fnRef.numEdges() != fnSel.numEdges())
            || (fnRef.numPolygons() != fnSel.numPolygons())
        ) {
            MString errorMsg("Reference mesh is not point compatible with selected mesh.");
            MGlobal::displayError(errorMsg);

            return MStatus::kFailure;
        }
    }

    return this->redoIt();
}

MStatus PolyFlipCommand::redoIt()
{
    if (this->worldSpace || this->objectSpace)
    {
        return this->flipMesh();
    } else if (this->referenceMesh.isValid()) {
        return this->flipMeshAgainst();
    } else {
        return MStatus::kSuccess;
    }
}


MStatus PolyFlipCommand::flipMesh()
{
    MStatus status;

    MSpace::Space space = this->worldSpace ? MSpace::kWorld : MSpace::kObject;

    vector<int> vertexSymmetry;
    vector<int> vertexSides;

    MFnDependencyNode fnNode(this->polySymmetryData);

    PolySymmetryNode::getValues(fnNode, VERTEX_SYMMETRY, vertexSymmetry);
    PolySymmetryNode::getValues(fnNode, VERTEX_SIDES, vertexSides);

    MFnMesh fnMesh(this->selectedMesh);
    MItGeometry itGeo(this->selectedMesh);
    itGeo.allPositions(this->originalPoints, space);

    uint numberOfVertices = fnMesh.numVertices();
    MPointArray newPoints(numberOfVertices);

    MPoint pnt;

    for (uint i = 0; i < numberOfVertices; i++)
    {
        int o = vertexSymmetry[i];
        pnt = this->originalPoints[o];

        newPoints.set(i, pnt.x * -1.0, pnt.y, pnt.z);
    }

    itGeo.setAllPositions(newPoints, space);

    return MStatus::kSuccess;
}

MStatus PolyFlipCommand::flipMeshAgainst()
{
    MStatus status;

    MSpace::Space space = this->worldSpace ? MSpace::kWorld : MSpace::kObject;

    vector<int> vertexSymmetry;
    vector<int> vertexSides;

    MPointArray referencePoints;
    MFnDependencyNode fnNode(this->polySymmetryData);

    PolySymmetryNode::getValues(fnNode, VERTEX_SYMMETRY, vertexSymmetry);
    PolySymmetryNode::getValues(fnNode, VERTEX_SIDES, vertexSides);

    MFnMesh fnMesh(this->selectedMesh);

    MItGeometry itRef(this->referenceMesh);
    MItGeometry itGeo(this->selectedMesh);

    itRef.allPositions(referencePoints, space);
    itGeo.allPositions(this->originalPoints, space);

    uint numberOfVertices = fnMesh.numVertices();
    MPointArray newPoints(numberOfVertices);

    MPoint pnt;
    MPoint ref;
    MVector delta;

    for (uint i = 0; i < numberOfVertices; i++)
    {
        if (vertexSides[i] == 0)
        {
            newPoints.set(this->originalPoints[i], i);
        }

        int o = vertexSymmetry[i];
        pnt = this->originalPoints[o];
        ref = referencePoints[o];

        delta = pnt - ref;

        ref = referencePoints[i];

        newPoints.set(
            i,
            (delta.x * -1.0) + ref.x,
            delta.y + ref.y,
            delta.z + ref.z
        );
    }

    itGeo.setAllPositions(newPoints, space);

    return MStatus::kSuccess;
}

MStatus PolyFlipCommand::undoIt()
{   
    MFnMesh fnMesh(this->selectedMesh);
    fnMesh.setPoints(this->originalPoints, MSpace::kObject);

    return MStatus::kSuccess;
}
