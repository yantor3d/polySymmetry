/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include <stdio.h>
#include <vector>

#include "polyDeformerWeights.h"
#include "polySymmetryNode.h"
#include "sceneCache.h"

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MFnWeightGeometryFilter.h>
#include <maya/MFloatArray.h>
#include <maya/MGlobal.h>
#include <maya/MItGeometry.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MPxCommand.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>

using namespace std;
 
// Indicates the source deformer.
#define SOURCE_DEFORMER_FLAG            "-sd"
#define SOURCE_DEFORMER_LONG_FLAG       "-sourceDeformer"

// Indicates the source deformed mesh.
#define SOURCE_MESH_FLAG               "-sm"
#define SOURCE_MESH_LONG_FLAG          "-sourceMesh"

// Indicates the destination deformer.
#define DESTINATION_DEFORMER_FLAG       "-dd"
#define DESTINATION_DEFORMER_LONG_FLAG  "-destinationDeformer"

// Indicates the destination deformed shape.
#define DESTINATION_MESH_FLAG          "-dm"
#define DESTINATION_MESH_LONG_FLAG     "-destinationMesh"

// Indicates the direction of the mirror or flip action - 1 (left to right) or -1 (right to left)
#define DIRECTION_FLAG                  "-d"
#define DIRECTION_LONG_FLAG             "-direction"

// Indicates that the deformer weights should be mirrored.
#define MIRROR_FLAG                     "-m"
#define MIRROR_LONG_FLAG                "-mirror"

// Indicates that the deformer weights should be flipped.
#define FLIP_FLAG                       "-f"
#define FLIP_LONG_FLAG                  "-flip"

#define RETURN_IF_ERROR(s) if (!s) { return s; }

PolyDeformerWeightsCommand::PolyDeformerWeightsCommand() {}
PolyDeformerWeightsCommand::~PolyDeformerWeightsCommand() {}

void* PolyDeformerWeightsCommand::creator()
{
    return new PolyDeformerWeightsCommand();
}

MSyntax PolyDeformerWeightsCommand::getSyntax()
{
    MSyntax syntax;

    syntax.addFlag(SOURCE_DEFORMER_FLAG, SOURCE_DEFORMER_LONG_FLAG, MSyntax::kSelectionItem);
    syntax.addFlag(SOURCE_MESH_FLAG, SOURCE_MESH_LONG_FLAG, MSyntax::kSelectionItem);

    syntax.addFlag(DESTINATION_DEFORMER_FLAG, DESTINATION_DEFORMER_LONG_FLAG, MSyntax::kSelectionItem);
    syntax.addFlag(DESTINATION_MESH_FLAG, DESTINATION_MESH_LONG_FLAG, MSyntax::kSelectionItem);

    syntax.addFlag(DIRECTION_FLAG, DIRECTION_LONG_FLAG, MSyntax::kLong);

    syntax.addFlag(MIRROR_FLAG, MIRROR_LONG_FLAG);
    syntax.addFlag(FLIP_FLAG, FLIP_LONG_FLAG);

    syntax.enableEdit(false);
    syntax.enableQuery(false);

    return syntax;
}

MStatus PolyDeformerWeightsCommand::parseArguments(MArgDatabase &argsData)
{
    MStatus status;

    status = getNodeArgument(argsData, SOURCE_DEFORMER_FLAG, this->sourceDeformer, true);
    RETURN_IF_ERROR(status);

    status = getNodeArgument(argsData, DESTINATION_DEFORMER_FLAG, this->destinationDeformer, false);
    RETURN_IF_ERROR(status);

    status = getDagPathArgument(argsData, SOURCE_MESH_FLAG, this->sourceMesh, true);
    RETURN_IF_ERROR(status);

    status = getDagPathArgument(argsData, DESTINATION_MESH_FLAG, this->destinationMesh, false);
    RETURN_IF_ERROR(status);

    this->mirrorWeights = argsData.isFlagSet(MIRROR_FLAG);
    this->flipWeights = argsData.isFlagSet(FLIP_FLAG);

    status = argsData.getFlagArgument(DIRECTION_FLAG, 0, this->direction);

    return MStatus::kSuccess;
}

MStatus PolyDeformerWeightsCommand::getNodeArgument(MArgDatabase &argsData, const char* flag, MObject &node, bool required)
{
    MStatus status;

    if (argsData.isFlagSet(flag))
    {
        MSelectionList selection;
        MString objectName;

        argsData.getFlagArgument(flag, 0, objectName);        
        selection.add(objectName);
        selection.getDependNode(0, node);
    } else if (required) {
        MString errorMsg("The ^1s flag is required.");
        errorMsg.format(errorMsg, MString(flag));
        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    return MStatus::kSuccess; 
}

MStatus PolyDeformerWeightsCommand::getDagPathArgument(MArgDatabase &argsData, const char* flag, MDagPath &path, bool required)
{
    MStatus status;

    MObject obj;
    status = getNodeArgument(argsData, flag, obj, required);

    if (status)
    {
        MDagPath::getAPathTo(obj, path);
    }

    return MStatus::kSuccess; 
}

MStatus PolyDeformerWeightsCommand::validateArguments()
{
    MStatus status;

    if (this->direction != 1 && this->direction != -1)
    {
        MString errorMsg("^1s/^2s flag should be 1 (left to right) or -1 (right to left)");
        errorMsg.format(errorMsg, MString(DIRECTION_LONG_FLAG), MString(DIRECTION_FLAG));

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    if (sourceDeformer.isNull() || !sourceDeformer.hasFn(MFn::kWeightGeometryFilt))
    {
        MString errorMsg("A deformer node should be specified with the ^1s/^2s flag.");
        errorMsg.format(errorMsg, MString(SOURCE_DEFORMER_LONG_FLAG), MString(SOURCE_DEFORMER_FLAG));

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    if (destinationDeformer.isNull()) 
    { 
        destinationDeformer = sourceDeformer; 
    } else if (!destinationDeformer.hasFn(MFn::kWeightGeometryFilt)) {
        MString errorMsg("A deformer node should be specified with the ^1s/^2s flag.");
        errorMsg.format(errorMsg, MString(DESTINATION_DEFORMER_LONG_FLAG), MString(DESTINATION_DEFORMER_FLAG));

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    if (sourceMesh.node().isNull() || !sourceMesh.hasFn(MFn::kMesh))
    {
        MString errorMsg("A mesh node should be specified with the ^1s/^2s flag.");
        errorMsg.format(errorMsg, MString(SOURCE_MESH_LONG_FLAG), MString(SOURCE_MESH_FLAG));
        
        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    if (destinationMesh.node().isNull()) 
    { 
        destinationMesh.set(sourceMesh); 
    } else if (!destinationMesh.hasFn(MFn::kMesh)) {
        MString errorMsg("A mesh node should be specified with the ^1s/^2s flag.");
        errorMsg.format(errorMsg, MString(DESTINATION_MESH_LONG_FLAG), MString(DESTINATION_MESH_FLAG));

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    } else {
        MFnMesh fnSourceMesh(this->sourceMesh);
        MFnMesh fnDestinationMesh(this->destinationMesh);
    
        if (fnSourceMesh.numVertices() != fnDestinationMesh.numVertices())
        {
            MString errorMsg("Source mesh and destination mesh are not point compatible. Cannot continue.");
            MGlobal::displayError(errorMsg);
            return MStatus::kFailure;
        }
    }

    if (!sourceMesh.node().hasFn(MFn::kMesh))
    {
        sourceMesh.extendToShapeDirectlyBelow(0);
    }

    if (!destinationMesh.node().hasFn(MFn::kMesh))
    {
        destinationMesh.extendToShapeDirectlyBelow(0);
    }

    if (mirrorWeights || flipWeights)
    {
        bool cacheHit = PolySymmetryCache::getNodeFromCache(this->sourceMesh, this->polySymmetryData);

        if (!cacheHit)
        {
            MString errorMsg("Mesh specified with the ^1s/^2s flag must have an associated ^3s node.");
            errorMsg.format(errorMsg, MString(SOURCE_MESH_LONG_FLAG), MString(SOURCE_MESH_FLAG), PolySymmetryNode::NODE_NAME);

            MGlobal::displayError(errorMsg);
            return MStatus::kFailure;
        }
    }

    return MStatus::kSuccess;
}

MStatus PolyDeformerWeightsCommand::doIt(const MArgList& argList)
{
    MStatus status;

    MArgDatabase argsData(syntax(), argList, &status);
    RETURN_IF_ERROR(status);

    status = this->parseArguments(argsData);
    RETURN_IF_ERROR(status);

    status = this->validateArguments();
    RETURN_IF_ERROR(status);

    return this->redoIt();
}

MStatus PolyDeformerWeightsCommand::redoIt()
{
    MStatus status;

    MFnWeightGeometryFilter fnSourceDeformer(this->sourceDeformer, &status);
    MFnWeightGeometryFilter fnDestinationDeformer(this->destinationDeformer, &status);

    MObjectArray sourceOutputGeometry;
    MObjectArray destinationOutputGeometry;

    int numberOfVertices = MFnMesh(this->sourceMesh).numVertices();

    MObject sourceComponents;
    MObject destinationComponents;

    this->getAllComponents(this->sourceMesh, sourceComponents);
    this->getAllComponents(this->destinationMesh, destinationComponents);

    MFloatArray sourceWeights(numberOfVertices);
    MFloatArray destinationWeights(numberOfVertices);

    oldWeightValues.setLength(numberOfVertices);
    
    uint sourceGeometryIndex = fnSourceDeformer.indexForOutputShape(this->sourceMesh.node(), &status);

    if (!status)
    {
        MString errorMsg("Source mesh ^1s is not deformed by deformer ^2s.");
        errorMsg.format(errorMsg, this->sourceMesh.partialPathName(), MFnDependencyNode(this->sourceDeformer).name());

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    uint destinationGeometryIndex = fnDestinationDeformer.indexForOutputShape(this->destinationMesh.node(), &status);

    if (!status)
    {
        MString errorMsg("Destination mesh ^1s is not deformed by deformer ^2s.");
        errorMsg.format(errorMsg, this->destinationMesh.partialPathName(), MFnDependencyNode(this->destinationDeformer).name());

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    status = fnSourceDeformer.getWeights(sourceGeometryIndex, sourceComponents, sourceWeights);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnDestinationDeformer.getWeights(destinationGeometryIndex, destinationComponents, oldWeightValues);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (mirrorWeights || flipWeights)
    {
        vector<int> vertexSymmetry;
        vector<int> vertexSides;

        MFnDependencyNode fnNode(polySymmetryData);

        PolySymmetryNode::getValues(fnNode, VERTEX_SYMMETRY, vertexSymmetry);
        PolySymmetryNode::getValues(fnNode, VERTEX_SIDES, vertexSides);

        for (int i = 0; i < numberOfVertices; i++)
        {
            int o = vertexSymmetry[i];
            float wt = sourceWeights[i];

            if (flipWeights || (mirrorWeights && vertexSides[i] != direction))
            {
                wt = sourceWeights[o];
            }

            destinationWeights.set(wt, i);
        }
    } else {
        destinationWeights.copy(sourceWeights);
    }

    status = fnDestinationDeformer.setWeight(this->destinationMesh, destinationGeometryIndex, destinationComponents, destinationWeights);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    this->geometryIndex = destinationGeometryIndex;
    this->components = destinationComponents;

    return MStatus::kSuccess;    
}

void PolyDeformerWeightsCommand::getAllComponents(MDagPath &mesh, MObject &components)
{
    MSelectionList selection;
    MItGeometry itGeo(mesh);

    while (!itGeo.isDone())
    {
        selection.add(mesh, itGeo.currentItem(), true);
        itGeo.next();
    }

    selection.getDagPath(0, mesh, components);
}

MStatus PolyDeformerWeightsCommand::undoIt()
{
    MStatus status;

    MFnWeightGeometryFilter fnDeformer(this->destinationDeformer, &status);

    status = fnDeformer.setWeight(
        this->destinationMesh,
        this->geometryIndex,
        this->components,
        this->oldWeightValues
    );

    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;
}