/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include <vector>

#include "polyMirrorCmd.h"
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

PolyMirrorCommand::PolyMirrorCommand()  {}
PolyMirrorCommand::~PolyMirrorCommand() {}

void* PolyMirrorCommand::creator()
{
    return new PolyMirrorCommand();
}

MSyntax PolyMirrorCommand::getSyntax()
{
    MSyntax syntax;

    syntax.setObjectType(MSyntax::kSelectionList, 2, 2);
    syntax.useSelectionAsDefault(true);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}

MStatus PolyMirrorCommand::doIt(const MArgList& argList)
{
    MStatus status;

    MArgDatabase argsData(syntax(), argList);

    MSelectionList selection;
    argsData.getObjects(selection);

    MStatus baseMeshStatus = selection.getDagPath(0, this->baseMesh);

    MStatus targetMeshStatus = selection.getDagPath(1, this->targetMesh);

    if (!this->baseMesh.hasFn(MFn::kMesh) || !this->targetMesh.hasFn(MFn::kMesh))
    {
        MGlobal::displayError("polyMirror command requires a a base mesh and a target mesh.");
        return MStatus::kFailure;
    }

    MFnMesh fnBaseMesh(this->baseMesh);
    MFnMesh fnTargetMesh(this->targetMesh);

    if (
        (fnBaseMesh.numVertices() != fnTargetMesh.numVertices())
        || (fnBaseMesh.numEdges() != fnTargetMesh.numEdges())
        || (fnBaseMesh.numPolygons() != fnTargetMesh.numPolygons())
    ) {
        MString errorMsg("Base mesh and target mesh are not point compatible.");
        MGlobal::displayError(errorMsg);

        return MStatus::kFailure;
    }

    bool cacheHit = PolySymmetryCache::getNodeFromCache(this->targetMesh, this->polySymmetryData);

    if (!cacheHit)
    {
        MString errorMsg("^1s has not had it's symmetry computed.");
        errorMsg.format(errorMsg, this->targetMesh.partialPathName());

        MGlobal::displayError(errorMsg);

        return MStatus::kFailure;
    }

    return this->redoIt();
}

MStatus PolyMirrorCommand::redoIt()
{
    vector<int> vertexSymmetry;
    vector<int> vertexSides;

    MFnDependencyNode fnNode(this->polySymmetryData);

    PolySymmetryNode::getValues(fnNode, VERTEX_SYMMETRY, vertexSymmetry);
    PolySymmetryNode::getValues(fnNode, VERTEX_SIDES, vertexSides);

    MPointArray basePoints;

    MItGeometry itBaseGeo(this->baseMesh);
    MItGeometry itTargetGeo(this->targetMesh);

    itBaseGeo.allPositions(basePoints, MSpace::kObject);
    itTargetGeo.allPositions(originalPoints, MSpace::kObject);

    int numberOfVertices = -1;
    PolySymmetryNode::getValue(fnNode, NUMBER_OF_VERTICES, numberOfVertices);
    MPointArray newPoints(numberOfVertices);

    MPoint basePnt;
    MPoint origPnt;
    MPoint newPnt;

    for (int i = 0; i < numberOfVertices; i++)
    {
        int o = vertexSymmetry[i];
        newPnt = originalPoints[o];

        newPoints.set(i, newPnt.x * -1.0, newPnt.y, newPnt.z);
    }
    
    for (int i = 0; i < numberOfVertices; i++)
    {
        origPnt = originalPoints[i];
        basePnt = basePoints[i];
        newPnt = newPoints[i];

        newPoints.set(
            i, 
            origPnt.x + newPnt.x - basePnt.x,
            origPnt.y + newPnt.y - basePnt.y,
            origPnt.z + newPnt.z - basePnt.z
        );
    }

    itTargetGeo.setAllPositions(newPoints, MSpace::kObject);

    return MStatus::kSuccess;
}

MStatus PolyMirrorCommand::undoIt()
{   
    MFnMesh fnMesh(this->targetMesh);
    fnMesh.setPoints(originalPoints, MSpace::kObject);

    return MStatus::kSuccess;
}
