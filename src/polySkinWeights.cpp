/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <regex>
#include <stdio.h>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "parseArgs.h"
#include "polySkinWeights.h"
#include "polySymmetryNode.h"
#include "sceneCache.h"
#include "selection.h"

#include "../pystring/pystring.h"

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnStringData.h>
#include <maya/MFnWeightGeometryFilter.h>
#include <maya/MFloatArray.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItGeometry.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MObjectHandle.h>
#include <maya/MPlug.h>
#include <maya/MPxCommand.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MStringArray.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>

using namespace std;
 
// Indicates the source deformed mesh.
#define SOURCE_MESH_FLAG               "-sm"
#define SOURCE_MESH_LONG_FLAG          "-sourceMesh"

// Indicates the source deformer.
#define SOURCE_SKIN_FLAG                "-ss"
#define SOURCE_SKIN_LONG_FLAG           "-sourceSkin"

// Indicates the destination deformed shape.
#define DESTINATION_MESH_FLAG           "-dm"
#define DESTINATION_MESH_LONG_FLAG      "-destinationMesh"

// Indicates the destination deformer.
#define DESTINATION_SKIN_FLAG           "-ds"
#define DESTINATION_SKIN_LONG_FLAG      "-destinationSkin"

// Indicates the direction of the mirror or flip action - 1 (left to right) or -1 (right to left)
#define DIRECTION_FLAG                  "-d"
#define DIRECTION_LONG_FLAG             "-direction"

// Indicates that the influences on the specified skinCluster should be tagged left/right/center 
#define INFLUENCE_SYMMETRY_FLAG         "-inf"
#define INFLUENCE_SYMMETRY_LONG_FLAG    "-influenceSymmetry"

// Indicates that the deformer weights should be mirrored.
#define MIRROR_FLAG                     "-m"
#define MIRROR_LONG_FLAG                "-mirror"

// Indicates that the skin weights should be normalized. 
#define NORMALIZE_FLAG                  "-nr"
#define NORMALIZE_LONG_FLAG             "-normalize"

// Indicates that the deformer weights should be flipped.
#define FLIP_FLAG                       "-f"
#define FLIP_LONG_FLAG                  "-flip"

#define CENTER_SIDE 0
#define LEFT_SIDE 1
#define RIGHT_SIDE 2
#define NONE_SIDE 3
#define OTHER_TYPE 18

#define RETURN_IF_ERROR(s) if (!s) { return s; }

PolySkinWeightsCommand::PolySkinWeightsCommand() {}
PolySkinWeightsCommand::~PolySkinWeightsCommand()
{
    influenceSymmetry.clear();
    oldWeights.clear();
    newWeights.clear();

    oldWeightValues.setLength(0);
}

void* PolySkinWeightsCommand::creator()
{
    return new PolySkinWeightsCommand();
}


MSyntax PolySkinWeightsCommand::getSyntax()
{
    MSyntax syntax;

    syntax.setObjectType(MSyntax::kSelectionList, 0, 1);
    syntax.useSelectionAsDefault(true);

    syntax.addFlag(SOURCE_MESH_FLAG, SOURCE_MESH_LONG_FLAG, MSyntax::kString);
    syntax.addFlag(SOURCE_SKIN_FLAG, SOURCE_SKIN_LONG_FLAG, MSyntax::kString);

    syntax.addFlag(DESTINATION_MESH_FLAG, DESTINATION_MESH_LONG_FLAG, MSyntax::kString);
    syntax.addFlag(DESTINATION_SKIN_FLAG, DESTINATION_SKIN_LONG_FLAG, MSyntax::kString);

    syntax.addFlag(INFLUENCE_SYMMETRY_FLAG, INFLUENCE_SYMMETRY_LONG_FLAG, MSyntax::kString, MSyntax::kString);
    syntax.makeFlagQueryWithFullArgs(INFLUENCE_SYMMETRY_FLAG, true);

    syntax.addFlag(DIRECTION_FLAG, DIRECTION_LONG_FLAG, MSyntax::kLong);
    syntax.addFlag(FLIP_FLAG, FLIP_LONG_FLAG);
    syntax.addFlag(MIRROR_FLAG, MIRROR_LONG_FLAG);
    syntax.addFlag(NORMALIZE_FLAG, NORMALIZE_LONG_FLAG);    

    syntax.enableQuery(true);
    syntax.enableEdit(true);

    return syntax;
}


/* Unpack the command arguments */
MStatus PolySkinWeightsCommand::parseArguments(MArgDatabase &argsData)
{
    MStatus status;

    status = parseArgs::getNodeArgument(argsData, SOURCE_SKIN_FLAG, this->sourceSkin, true);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = parseArgs::getNodeArgument(argsData, DESTINATION_SKIN_FLAG, this->destinationSkin, false);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = parseArgs::getDagPathArgument(argsData, SOURCE_MESH_FLAG, this->sourceMesh, true);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = parseArgs::getDagPathArgument(argsData, DESTINATION_MESH_FLAG, this->destinationMesh, false);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (argsData.isFlagSet(INFLUENCE_SYMMETRY_FLAG))
    {
        this->parseInfluenceSymmetryArgument(argsData);
    }

    this->direction = argsData.isFlagSet(DIRECTION_FLAG) ? argsData.flagArgumentInt(DIRECTION_FLAG, 0, &status) : 1;

    this->mirrorWeights = argsData.isFlagSet(MIRROR_FLAG);
    this->flipWeights = argsData.isFlagSet(FLIP_FLAG);
    this->normalizeWeights = argsData.isFlagSet(NORMALIZE_FLAG);

    return MStatus::kSuccess;
}


/* Unpack the edit arguments */
MStatus PolySkinWeightsCommand::parseEditArguments(MArgDatabase &argsData)
{
    MStatus status;
    
    status = this->parseQueryArguments(argsData);
    RETURN_IF_ERROR(status);

    if (!argsData.isFlagSet(INFLUENCE_SYMMETRY_FLAG))
    {
        MString errorMsg("^1s/^2s flag is required in edit mode.");
        errorMsg.format(errorMsg, MString(INFLUENCE_SYMMETRY_LONG_FLAG), MString(INFLUENCE_SYMMETRY_FLAG));

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    this->parseInfluenceSymmetryArgument(argsData);

    return MStatus::kSuccess;
}


/* Unpack the query arguments */
MStatus PolySkinWeightsCommand::parseQueryArguments(MArgDatabase &argsData)
{
    MStatus status;

    MSelectionList selection;
    argsData.getObjects(selection);

    MDagPath dagPath;
    MObject object;

    status = selection.getDagPath(0, dagPath);

    if (!argsData.isFlagSet(INFLUENCE_SYMMETRY_FLAG))
    {
        MString errorMsg("The ^1s/^2s flag is required in query mode.");
        errorMsg.format(
            errorMsg, 
            MString(INFLUENCE_SYMMETRY_LONG_FLAG),
            MString(INFLUENCE_SYMMETRY_FLAG)
        );

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    this->parseInfluenceSymmetryArgument(argsData);

    if (status && parseArgs::isNodeType(dagPath, MFn::kMesh))
    {
        if (!dagPath.node().hasFn(MFn::kMesh))
        {
            dagPath.extendToShapeDirectlyBelow(0);
        }

        object = dagPath.node();

        MItDependencyGraph iterGraph(
            object, 
            MFn::kSkinClusterFilter,
            MItDependencyGraph::kUpstream,
            MItDependencyGraph::kDepthFirst,
            MItDependencyGraph::kNodeLevel,
            &status
        );

        CHECK_MSTATUS_AND_RETURN_IT(status);
        
        while (!iterGraph.isDone())
        {
            MObject obj = iterGraph.currentItem();

            if (obj.hasFn(MFn::kSkinClusterFilter))
            {
                this->sourceSkin = obj;
                break;
            }
            iterGraph.next();
        }
    } else {
        status = selection.getDependNode(0, object);

        if (status && parseArgs::isNodeType(object, MFn::kSkinClusterFilter))
        {
            this->sourceSkin = object;
        }
    }

    return MStatus::kSuccess;
}

MStatus PolySkinWeightsCommand::parseInfluenceSymmetryArgument(MArgDatabase &argsData)
{
    MStatus status;

    if (argsData.isFlagSet(INFLUENCE_SYMMETRY_FLAG))
    {
        MString leftInfluencePatternArg;
        MString rightInfluencePatternArg;

        MStatus s0 = argsData.getFlagArgument(INFLUENCE_SYMMETRY_FLAG, 0, leftInfluencePatternArg);
        MStatus s1 = argsData.getFlagArgument(INFLUENCE_SYMMETRY_FLAG, 1, rightInfluencePatternArg);

        if (s0 && s1)
        {
            leftInfluencePattern = string(leftInfluencePatternArg.asChar());
            rightInfluencePattern = string(rightInfluencePatternArg.asChar());

            isInfluenceSymmetryFlagSet = true;
        } else {
            isQueryInfluenceSymmetry = this->isQuery;
        }        
    }

    return MStatus::kSuccess;
}


/* Validate the command arguments */
MStatus PolySkinWeightsCommand::validateArguments()
{
    MStatus status;

    if (this->isInfluenceSymmetryFlagSet)
    {
        status = this->validateInfluenceSymmetryArgument();
        RETURN_IF_ERROR(status);
    }

    // Direction msut be left to right (1) or right to left (-1)
    if (this->direction != 1 && this->direction != -1)
    {
        MString errorMsg("^1s/^2s flag should be 1 (left to right) or -1 (right to left)");
        errorMsg.format(errorMsg, MString(DIRECTION_LONG_FLAG), MString(DIRECTION_FLAG));

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    // Source mesh must be a mesh
    if (!parseArgs::isNodeType(this->sourceMesh, MFn::kMesh))
    {
        MString errorMsg("A mesh node should be specified with the ^1s/^2s flag.");
        errorMsg.format(errorMsg, MString(SOURCE_MESH_LONG_FLAG), MString(SOURCE_MESH_FLAG));
        
        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    // Destination mesh must be a mesh, and be point compatible with the source mesh.
    // If no destination mesh is specified, use the source mesh instead.
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

    // Source/destination mesh must have polySymmetryData cached if the weights are going to be mirrored or flipped.
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
    
    this->numberOfVertices = MFnMesh(sourceMesh).numVertices();

    MSelectionList activeSelection;
    MSelectionList vertexSelection;

    MGlobal::getActiveSelectionList(activeSelection);

    if (destinationMesh.node().hasFn(MFn::kMesh)) { destinationMesh.pop(); }
    getSelectedComponents(destinationMesh, activeSelection, vertexSelection, MFn::Type::kMeshVertComponent);

    MObject selectedVertices;

    if (!vertexSelection.isEmpty())
    {
        vertexSelection.getDagPath(0, destinationMesh, selectedVertices);
    }

    MItGeometry itGeo(destinationMesh, selectedVertices);
    uint numSelectedVertices = itGeo.exactCount();

    vector<bool> vertexIsSelected(numberOfVertices);
    selectedVertexIndices.resize(numberOfVertices, -1);

    int idx = 0;

    while (!itGeo.isDone())
    {
        int i = itGeo.index();
        selectedVertexIndices[idx++] = i;
        vertexIsSelected[i] = true;
        itGeo.next();
    }

    if (!selectedVertices.isNull() && !polySymmetryData.isNull())
    {
        vector<int> vertexSymmetry;

        MFnDependencyNode fnNode(this->polySymmetryData);
        PolySymmetryNode::getValues(fnNode, VERTEX_SYMMETRY, vertexSymmetry);

        for (uint i = 0; i < numSelectedVertices; i++)
        {
            int v = selectedVertexIndices[i];
            int sv = vertexSymmetry[v];

            if (!vertexIsSelected[sv])
            {
                selectedVertexIndices[idx++] = sv;
            }
        }
    }

    selectedVertexIndices.resize(idx);

    if (!sourceMesh.node().hasFn(MFn::kMesh))
    {
        sourceMesh.extendToShapeDirectlyBelow(0);
    }

    if (!destinationMesh.node().hasFn(MFn::kMesh))
    {
        destinationMesh.extendToShapeDirectlyBelow(0);
    }

    // Source skin must be a skinCluster.
    if (!parseArgs::isNodeType(sourceSkin, MFn::kSkinClusterFilter))
    {
        MString errorMsg("A skinCluster node should be specified with the ^1s/^2s flag.");
        errorMsg.format(errorMsg, MString(SOURCE_SKIN_LONG_FLAG), MString(SOURCE_SKIN_FLAG));

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    // Destination skin must be a skinCluster.
    // If no destination skin is specified, use the source skin.
    if (destinationSkin.isNull()) 
    { 
        if (this->sourceMesh == this->destinationMesh)
        {
            destinationSkin = sourceSkin; 
        }
    }
    
    if (destinationSkin.isNull() || !destinationSkin.hasFn(MFn::kSkinClusterFilter)) 
    {
        MString errorMsg("A skinCluster node should be specified with the ^1s/^2s flag.");
        errorMsg.format(errorMsg, MString(DESTINATION_SKIN_LONG_FLAG), MString(DESTINATION_SKIN_FLAG));

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    // Source mesh must be deformed by the source skin cluster.
    if (!isDeformedBy(sourceSkin, sourceMesh))
    {
        MString errorMsg("^1s is not deformed by ^2s.");
        errorMsg.format(errorMsg, sourceMesh.partialPathName(), MFnSkinCluster(sourceSkin).name());

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    // Destination mesh must be deformed by the destination skin cluster.
    if (!isDeformedBy(destinationSkin, destinationMesh))
    {
        MString errorMsg("^1s is not deformed by ^2s.");
        errorMsg.format(errorMsg, destinationMesh.partialPathName(), MFnSkinCluster(destinationSkin).name());

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

    return MStatus::kSuccess;
}


/* Validate the query arguments. */
MStatus PolySkinWeightsCommand::validateEditArguments()
{
    MStatus status;

    if (this->sourceSkin.isNull())
    {
        MGlobal::displayError("polySkinWeights -edit requires a mesh or skinCluster ");
        return MStatus::kFailure;
    }

    status = this->validateInfluenceSymmetryArgument();
    RETURN_IF_ERROR(status);

    return MStatus::kSuccess;
}


/* Validate the query arguments. */
MStatus PolySkinWeightsCommand::validateQueryArguments()
{
    MStatus status;

    if (this->sourceSkin.isNull())
    {
        MGlobal::displayError("polySkinWeights -query requires a mesh or skinCluster ");
        return MStatus::kFailure;
    }

    if (isInfluenceSymmetryFlagSet)
    {
        status = validateInfluenceSymmetryArgument();
        RETURN_IF_ERROR(status);
    }

    return MStatus::kSuccess;
}


MStatus PolySkinWeightsCommand::validateInfluenceSymmetryArgument()
{
    MStatus status;

    if (isInfluenceSymmetryFlagSet)
    {
        if (leftInfluencePattern.empty() || rightInfluencePattern.empty())
        {
            MString errorMsg("^1s/^2s patterns must not be empty strings.");

            errorMsg.format(
                errorMsg,
                MString(INFLUENCE_SYMMETRY_LONG_FLAG), 
                MString(INFLUENCE_SYMMETRY_FLAG)
            );

            MGlobal::displayError(errorMsg);
            return MStatus::kFailure;  
        }

        bool leftPatternIsValid = pystring::index(leftInfluencePattern, string("*")) != -1;
        bool rightPatternIsValid = pystring::index(rightInfluencePattern, string("*")) != -1;

        if (!leftPatternIsValid || !rightPatternIsValid)
        {
            MString errorMsg("^1s/^2s patterns must have at least one '*' wildcard.");

            errorMsg.format(
                errorMsg,
                MString(INFLUENCE_SYMMETRY_LONG_FLAG), 
                MString(INFLUENCE_SYMMETRY_FLAG)
            );

            MGlobal::displayError(errorMsg);
            return MStatus::kFailure;
        }
    }

    return MStatus::kSuccess;
}


bool PolySkinWeightsCommand::isDeformedBy(MObject &skin, MDagPath &mesh)
{
    bool result = false;

    MObjectArray outputGeometry;
    MFnSkinCluster(skin).getOutputGeometry(outputGeometry);

    for (uint i = 0; i < outputGeometry.length(); i++)
    {
        if (mesh.node() == outputGeometry[i])
        {
            result = true;
            break;
        }
    }

    return result;
}


MStatus PolySkinWeightsCommand::doIt(const MArgList& argList)
{
    MStatus status;
    MArgDatabase argsData(syntax(), argList, &status);
    RETURN_IF_ERROR(status);

    this->isQuery = argsData.isQuery();
    this->isEdit = argsData.isEdit();

    if (this->isQuery)
    {
        status = this->parseQueryArguments(argsData);
        RETURN_IF_ERROR(status);

        status = this->validateQueryArguments();
        RETURN_IF_ERROR(status);
    } else if (this->isEdit) {
        status = this->parseEditArguments(argsData);
        RETURN_IF_ERROR(status);

        status = this->validateEditArguments();
        RETURN_IF_ERROR(status);
    } else {
        status = this->parseArguments(argsData);
        RETURN_IF_ERROR(status);

        status = this->validateArguments();
        RETURN_IF_ERROR(status);
    }

    return this->redoIt();
}


MStatus PolySkinWeightsCommand::redoIt()
{
    if (this->isQuery)
    {
        return this->queryPolySkinWeights();
    } else if (this->isEdit) {
        return this->editPolySkinWeights();
    } else {
        return this->copyPolySkinWeights();        
    }
}


/* Do the query command */
MStatus PolySkinWeightsCommand::queryPolySkinWeights()
{
    MStatus status;

    MFnSkinCluster fnSkin(this->sourceSkin);
    MDagPathArray influences;
    vector<string> influenceKeys;

    uint numberOfInfluences = fnSkin.influenceObjects(influences);
   
    this->getInfluenceKeys(fnSkin, influenceKeys);

    unordered_map<string, JointLabel> jointLabels;
    getJointLabels(influences, influenceKeys, jointLabels);

    this->makeInfluenceSymmetryTable(influences, influenceKeys, jointLabels);

    MStringArray results;

    unordered_map<string, MDagPath> influencesMap;

    for (uint i = 0; i < numberOfInfluences; i++)
    {
        influencesMap.emplace(influenceKeys[i], influences[i]);
    }

    for (uint i = 0; i < numberOfInfluences; i++)
    {
        MString pair("^1s:^2s");

        string lhsKey = influenceKeys[i];
        string rhsKey = influenceSymmetry[lhsKey];

        MString lhs = influencesMap[lhsKey].partialPathName();
        MString rhs = influencesMap[rhsKey].partialPathName();

        pair.format(pair, lhs, rhs);
        
        this->appendToResult(pair);
    }

    return MStatus::kSuccess;  
}


/* Do the edit command */
MStatus PolySkinWeightsCommand::editPolySkinWeights()
{
    MStatus status;

    MFnSkinCluster fnSkin(this->sourceSkin);
    MDagPathArray influences;
    vector<string> influenceKeys;

    uint numberOfInfluences = fnSkin.influenceObjects(influences);
    getInfluenceKeys(fnSkin, influenceKeys);
   
    unordered_map<string, JointLabel> jointLabels;
    unordered_map<string, JointLabel> newJointLabels;

    getJointLabels(influences, influenceKeys, jointLabels);
    
    for (uint i = 0; i < numberOfInfluences; i++)
    {
        MString influencePathName = influences[i].partialPathName();
        string influenceName(influencePathName.asChar());

        if (!influences[i].hasFn(MFn::kJoint))
        {
            MString warning("Cannot set '^1s' .side, .type, and .otherType attributes because it is not a joint.");
            warning.format(warning, influencePathName);

            MGlobal::displayWarning(warning);
            continue;
        }

        string key = influenceKeys[i];

        JointLabel oldJointLabel = getJointLabel(influences[i]);
        JointLabel newJointLabel = jointLabels[key];

        oldJointLabels.emplace(key, oldJointLabel);    
        newJointLabels.emplace(key, newJointLabel);
            
        setJointLabel(influences[i], newJointLabel);
    }

    return MStatus::kSuccess;  
}


/* Do the command */
MStatus PolySkinWeightsCommand::copyPolySkinWeights()
{
    MStatus status;

    MFnSkinCluster fnSourceSkin(this->sourceSkin);
    MFnSkinCluster fnDestinationSkin(this->destinationSkin);

    status = this->makeInfluencesMatch(fnSourceSkin, fnDestinationSkin);
    RETURN_IF_ERROR(status);

    MDagPathArray   sourceInfluences;
    MIntArray       sourceInfluenceIndices;
    vector<string>  sourceInfluenceKeys;
    MDoubleArray    sourceWeights;
    uint            numSourceInfluences;

    MDagPathArray   destinationInfluences;
    MIntArray       destinationInfluenceIndices;
    vector<string>  destinationInfluenceKeys;
    MDoubleArray    destinationWeights;
    uint            numDestinationInfluences;

    numSourceInfluences = fnSourceSkin.influenceObjects(sourceInfluences);
    this->getInfluenceIndices(fnSourceSkin, sourceInfluenceIndices);
    this->getInfluenceKeys(fnSourceSkin, sourceInfluenceKeys);
    getAllComponents(sourceMesh, sourceComponents);

    numDestinationInfluences = fnDestinationSkin.influenceObjects(destinationInfluences);
    this->getInfluenceIndices(fnDestinationSkin, destinationInfluenceIndices);
    this->getInfluenceKeys(fnDestinationSkin, destinationInfluenceKeys);
    getAllComponents(destinationMesh, destinationComponents);

    unordered_map<string, JointLabel> jointLabels;
    getJointLabels(destinationInfluences, destinationInfluenceKeys, jointLabels);

    this->makeInfluenceSymmetryTable(destinationInfluences, destinationInfluenceKeys, jointLabels);    
    this->makeWeightTables(destinationInfluenceKeys);

    status = fnSourceSkin.getWeights(
        sourceMesh, 
        sourceComponents, 
        sourceInfluenceIndices,
        sourceWeights
    );
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnDestinationSkin.getWeights(
        destinationMesh, 
        destinationComponents, 
        destinationInfluenceIndices, 
        destinationWeights
    );
    CHECK_MSTATUS_AND_RETURN_IT(status);

    this->setWeightsTable(this->oldWeights, sourceWeights, sourceInfluenceKeys);

    if (flipWeights)
    {
        this->flipWeightsTable(destinationInfluenceKeys);
    } else if (mirrorWeights) {
        this->mirrorWeightsTable(destinationInfluenceKeys);
    } else {
        this->copyWeightsTable(destinationInfluenceKeys);
    }
    
    this->getWeightsTable(this->newWeights, destinationWeights, destinationInfluenceKeys);

    status = fnDestinationSkin.setWeights(
        this->destinationMesh,
        this->destinationComponents,
        destinationInfluenceIndices,
        destinationWeights,
        this->normalizeWeights,
        &this->oldWeightValues
    );

    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;    
}


/* Ensures that the destination skin cluster has all the influences present on the source skin cluster. */
MStatus PolySkinWeightsCommand::makeInfluencesMatch(MFnSkinCluster &fnSourceSkin, MFnSkinCluster &fnDestinationSkin)
{
    MStatus status;

    MDagPathArray sourceInfluenceObjects;
    MDagPathArray destinationInfluenceObjects;

    uint numberOfSourceInfluences = fnSourceSkin.influenceObjects(sourceInfluenceObjects, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    uint numberOfDestinationInfluences = fnDestinationSkin.influenceObjects(destinationInfluenceObjects, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MDagPathArray missingInfluences;

    for (uint i = 0; i < numberOfSourceInfluences; i++)
    {
        bool isMissingInfluence = true;

        for (uint j = 0; j < numberOfDestinationInfluences; j++)
        {
            if (sourceInfluenceObjects[i] == destinationInfluenceObjects[j])
            {
                isMissingInfluence = false;
                break;
            }
        }

        if (isMissingInfluence)
        {
            missingInfluences.append(sourceInfluenceObjects[i]);
        }
    }

    if (missingInfluences.length() != 0)
    {
        MString destinationSkinName = MFnDependencyNode(this->destinationSkin).name();
        MString addMissingInfluencesCmd("skinCluster -edit -lockWeights false -weight 0.0");

        for (uint i = 0; i < missingInfluences.length(); i++)
        {
            MString influenceName = missingInfluences[i].partialPathName();

            if (missingInfluences[i].hasFn(MFn::kShape))
            {
                if (missingInfluences[i].node().hasFn(MFn::kTransform))
                {
                    missingInfluences[i].extendToShapeDirectlyBelow(0);
                }

                MString addMissingGeometryInfluenceCmd(
                    "skinCluster -edit -lockWeights false -weight 0.0 -addInfluence ^1s -useGeometry -baseShape ^2s -polySmoothness 0 -nurbsSamples 10 -dropoffRate 4 ^3s;"
                );

                addMissingGeometryInfluenceCmd.format(
                    addMissingGeometryInfluenceCmd, 
                    influenceName,
                    missingInfluences[i].partialPathName(),
                    destinationSkinName
                );

                dgModifier.commandToExecute(addMissingGeometryInfluenceCmd);
                continue;
            } else {
                addMissingInfluencesCmd += " -addInfluence " + influenceName;
            }
        }

        addMissingInfluencesCmd += " " + destinationSkinName + ";";        
        dgModifier.commandToExecute(addMissingInfluencesCmd);

        status = dgModifier.doIt();
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    fnDestinationSkin.setObject(this->destinationSkin);

    return MStatus::kSuccess;
}


/* Constructs an influence->influence symmetry map. */
MStatus PolySkinWeightsCommand::makeInfluenceSymmetryTable(
    MDagPathArray &influences, 
    vector<string> &influenceKeys,
    unordered_map<string, JointLabel> &jointLabels
) {
    MStatus status;

    uint numberOfInfluences = influences.length();

    for (string key : influenceKeys)
    {
        this->influenceSymmetry.emplace(key, key);
    }

    for (uint i = 0; i < numberOfInfluences; i++)
    {
        string thisInfluence = influenceKeys[i];
        JointLabel thisLabel = jointLabels[thisInfluence];

        for (uint j = 0; j < numberOfInfluences; j++)
        {
            bool isTaggedCenter = thisLabel.side == CENTER_SIDE || thisLabel.side == NONE_SIDE;

            if ((i == j) && isTaggedCenter)
            {
                this->influenceSymmetry[thisInfluence] = thisInfluence;
                break;
            } else if (i == j) {
                continue;
            }

            string otherInfluence = influenceKeys[j];
            JointLabel otherLabel = jointLabels[otherInfluence];

            bool leftToRight = (thisLabel.side == LEFT_SIDE && otherLabel.side == RIGHT_SIDE);
            bool rightToLeft = (thisLabel.side == RIGHT_SIDE && otherLabel.side == LEFT_SIDE);

            if (leftToRight || rightToLeft)
            {                
                if (thisLabel.type == OTHER_TYPE && otherLabel.type == OTHER_TYPE)
                {
                    if (thisLabel.otherType == otherLabel.otherType)
                    {
                        this->influenceSymmetry[thisInfluence] = otherInfluence;
                        break;
                    }
                } else if (thisLabel.type == otherLabel.type) {
                    this->influenceSymmetry[thisInfluence] = otherInfluence;
                    break;
                }
            }
        }
    }

    return MStatus::kSuccess;
}


/* Constructs a pair of maps (influence->weights) from the influences of the desination skin cluster. */
void PolySkinWeightsCommand::makeWeightTables(vector<string> &influenceKeys) 
{
    uint numberOfInfluences = (uint) influenceKeys.size();

    for (uint i = 0; i < numberOfInfluences; i++)
    {
        string key = influenceKeys[i];
        oldWeights.emplace(key, vector<double>(this->numberOfVertices));
        newWeights.emplace(key, vector<double>(this->numberOfVertices));
    }
}


/* Copies the weights from the weight array to the weight table. */
void PolySkinWeightsCommand::setWeightsTable(unordered_map<string, vector<double>> &weightTable, MDoubleArray &weights, vector<string> &influenceKeys)
{
    uint numberOfInfluences = (uint) influenceKeys.size();
    
    for (uint i = 0; i < this->numberOfVertices; i++)
    {
        for (uint j = 0; j < numberOfInfluences; j++)
        {
            uint wi = (i * numberOfInfluences) + j;
            oldWeights[influenceKeys[j]][i] = weights[wi];
        }
    }
}

/* Copies the weights from the weight table to the weight array. */
void PolySkinWeightsCommand::getWeightsTable(unordered_map<string, vector<double>> &weightTable, MDoubleArray &weights, vector<string> &influenceKeys)
{
    uint numberOfInfluences = (uint) influenceKeys.size();

    weights.setLength(this->numberOfVertices * numberOfInfluences);

    for (int i : selectedVertexIndices)
    {
        for (uint j = 0; j < numberOfInfluences; j++)
        {
            string key = influenceKeys[j];
            uint wi = (i * numberOfInfluences) + j;
            
            bool hasKey = weightTable.find(key) != weightTable.end();

            double wt = hasKey ? weightTable[key][i] : 0.0;

            weights.set(wt, wi);
        }
    }
}


/* Returns the physical indices of the skin cluster influences. */
MStatus PolySkinWeightsCommand::getInfluenceIndices(MFnSkinCluster &fnSkin, MIntArray &influenceIndices)
{
    MStatus status;

    MDagPathArray influences;
    uint numberOfInfluences = fnSkin.influenceObjects(influences);

    influenceIndices.setLength(numberOfInfluences);

    for (uint i = 0; i < numberOfInfluences; i++)
    {
        /*
            MFnSkinCluster::indexForInfluenceObject returns the LOGICAL index of an influence.
            MFnSkinCluster::setWeights and MFnSkinCluster::getWeights require the PHYSICAL index of an influence.

            This is why we can't have nice things.
        */

        influenceIndices.set((int) i, i);
    }

    return MStatus::kSuccess;
}

/* Returns hash keys for the influences on the skin cluster. */
MStatus PolySkinWeightsCommand::getInfluenceKeys(MFnSkinCluster &fnSkin, vector<string> &influenceKeys)
{
    MStatus status;
    
    MDagPathArray influences;
    uint numberOfInfluences = fnSkin.influenceObjects(influences);

    influenceKeys.resize(numberOfInfluences);

    for (uint i = 0; i < numberOfInfluences; i++)
    {
        string key = string(influences[i].fullPathName().asChar());
        influenceKeys[i] = key;
    }

    return MStatus::kSuccess;
}

/* Copy the weights from the old weights table to the new weights table */
void PolySkinWeightsCommand::copyWeightsTable(vector<string> &influenceKeys)
{
    for (string ii : influenceKeys)
    {
        for (uint i = 0; i < numberOfVertices; i++)
        {
            newWeights[ii][i] = oldWeights[ii][i];
        }
    }
}


/* Copy the weights from the old weights table indices to the opposite indices in the new weights table. */
void PolySkinWeightsCommand::flipWeightsTable(vector<string> &influenceKeys)
{
    vector<int> vertexSymmetry;

    MFnDependencyNode fnNode(this->polySymmetryData);
    PolySymmetryNode::getValues(fnNode, VERTEX_SYMMETRY, vertexSymmetry);

    for (string ii : influenceKeys)
    {
        for (int i : selectedVertexIndices)
        {
            int o = vertexSymmetry[i];

            newWeights[ii][i] = oldWeights[ii][o];
            newWeights[ii][o] = oldWeights[ii][i];
        }
    }
}


/* Copy the weights from the old weights table indices to the same and opposite indices in the new weights table. */
void PolySkinWeightsCommand::mirrorWeightsTable(vector<string> &influenceKeys)
{
    vector<int> vertexSymmetry;
    vector<int> vertexSides;

    MFnDependencyNode fnNode(this->polySymmetryData);
    PolySymmetryNode::getValues(fnNode, VERTEX_SYMMETRY, vertexSymmetry);
    PolySymmetryNode::getValues(fnNode, VERTEX_SIDES, vertexSides);
    
    for (string ii : influenceKeys)
    {
        if (newWeights.find(ii) == newWeights.end())
        {
            continue;
        }

        for (int i : selectedVertexIndices)
        {
            int o = vertexSymmetry[i];
            string oi = influenceSymmetry[ii];

            if (vertexSides[i] == CENTER_SIDE)
            {
                if (oldWeights.find(oi) != oldWeights.end() && ii != oi)
                {
                    newWeights[ii][i] = oldWeights[ii][i];
                    newWeights[oi][i] = oldWeights[ii][i];
                } else {
                    newWeights[ii][i] = oldWeights[ii][i];
                }
            } else if (vertexSides[i] == direction) {
                newWeights[ii][i] = oldWeights[ii][i];
            } else {
                if (oldWeights.find(oi) != oldWeights.end()) 
                {
                    newWeights[ii][i] = oldWeights[oi][o];
                } else {
                    newWeights[ii][i] = 0.0;
                }
            }
        }
    }

    for (int i : selectedVertexIndices)
    {
        double centerWeightMax = 0.0;
        double centerWeights = 0.0;
        double sideWeights = 0.0;

        for (string ii : influenceKeys)
        {
            string oi = influenceSymmetry[ii];

            double wt = newWeights[ii][i];

            if (fabs(wt) < 1e-5)
            {
                continue;
            }

            if (ii == oi)
            {
                centerWeights += wt;
                centerWeightMax = max(centerWeightMax, wt);
            } else {
                sideWeights += wt;
            }
        }

        double sumOfWeights = centerWeights + sideWeights;

        if (sumOfWeights > 1.0 && centerWeights > 1e-5)
        {
            double sideMult = (1.0 - centerWeights) * sumOfWeights;    
            double centerMult = max(0.0, 1.0 - (sideWeights * sideMult)) * (1.0 / centerWeightMax);

            for (string ii : influenceKeys)
            {
                string oi = influenceSymmetry[ii];

                if (ii != oi)
                {
                    newWeights[ii][i] *= sideMult;
                }
            }            

            for (string ii : influenceKeys)
            {
                string oi = influenceSymmetry[ii];

                if (ii == oi)
                {
                    newWeights[ii][i] *= centerMult;
                }
            }
        }
    }
}


/* Populates the jointLabels map. */
void PolySkinWeightsCommand::getJointLabels(MDagPathArray &influences, vector<string> &influenceKeys, unordered_map<string, JointLabel> &jointLabels)
{
    uint numberOfInfluences = influences.length();

    if (this->isInfluenceSymmetryFlagSet)
    {
        string wildcard = string("*");
        string regexAny = string(".*");
        string emptyString = string("");

        string leftToken = pystring::replace(leftInfluencePattern, regexAny, emptyString);
        string rightToken = pystring::replace(rightInfluencePattern, regexAny, emptyString);

        leftToken = pystring::replace(leftToken, wildcard, emptyString);
        rightToken = pystring::replace(rightToken, wildcard, emptyString);

        string leftRegexPattern = pystring::replace(leftInfluencePattern, regexAny, wildcard);
        string rightRegexPattern = pystring::replace(rightInfluencePattern, regexAny, wildcard);

        leftRegexPattern = pystring::replace(leftRegexPattern, wildcard, regexAny);
        rightRegexPattern = pystring::replace(rightRegexPattern, wildcard, regexAny);

        regex leftRegex(leftRegexPattern);
        regex rightRegex(rightRegexPattern);

        for (uint i = 0; i < numberOfInfluences; i++)
        {
            MString influencePathName = influences[i].partialPathName();
            string influenceName(influencePathName.asChar());

            JointLabel newJointLabel;

            bool isLeft = regex_match(influenceName, leftRegex);
            bool isRight = regex_match(influenceName, rightRegex);

            if (isLeft) 
            {
                influenceName = pystring::replace(influenceName, leftToken, emptyString);
                newJointLabel.side = LEFT_SIDE;
            } else if (isRight) {
                influenceName = pystring::replace(influenceName, rightToken, emptyString);
                newJointLabel.side = RIGHT_SIDE;            
            } else {
                newJointLabel.side = CENTER_SIDE;
            }

            newJointLabel.type = OTHER_TYPE;
            newJointLabel.otherType = MString(influenceName.c_str());

            jointLabels.emplace(influenceKeys[i], newJointLabel);
        }
    } else {
        for (uint i = 0; i < numberOfInfluences; i++)
        {
            string key = influenceKeys[i];
            JointLabel lbl = this->getJointLabel(influences[i]);

            jointLabels.emplace(key, lbl);
        }
    }
}


/* Return the side, type, and otherType values for the given influence. */
JointLabel PolySkinWeightsCommand::getJointLabel(MDagPath &influence)
{
    // TODO - how to handle non-joint objects?
    MStatus status;
    JointLabel result;

    MFnDependencyNode fnNode(influence.node());

    MPlug sidePlug = fnNode.findPlug("side", false, &status);
    if (status) { result.side = sidePlug.asInt(); }
    CHECK_MSTATUS(status);

    MPlug typePlug = fnNode.findPlug("type", false, &status);
    if (status) { result.type = typePlug.asInt(); }
    CHECK_MSTATUS(status);

    MPlug otherTypePlug = fnNode.findPlug("otherType", false, &status); 
    if (status && result.type == OTHER_TYPE) { result.otherType = otherTypePlug.asString(); }
    CHECK_MSTATUS(status);

    return result;
}


/* Set the side, type, and otherType attributes to the values in `jointLabel`. */
MStatus PolySkinWeightsCommand::setJointLabel(MDagPath &influence, JointLabel &jointLabel)
{
    MStatus status;

    MFnDependencyNode fnNode(influence.node());

    MPlug sidePlug = fnNode.findPlug("side", false, &status);
    if (status) 
    {
        status = sidePlug.setValue(jointLabel.side);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else {
        CHECK_MSTATUS(status);
    }

    MPlug typePlug = fnNode.findPlug("type", false, &status);
    if (status) 
    {
        status = typePlug.setValue(jointLabel.type);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else {
        CHECK_MSTATUS(status);
    }            

    MPlug otherTypePlug = fnNode.findPlug("otherType", false, &status); 
    if (status) 
    {
        status = otherTypePlug.setValue(jointLabel.otherType);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    } else {
        CHECK_MSTATUS(status);
    }
    
    return MStatus::kSuccess;
}


MStatus PolySkinWeightsCommand::undoIt()
{
    if (this->isEdit)
    {
        return this->undoEditPolySkinWeights();
    } else {
        return this->undoCopyPolySkinWeights();
    }
}


/* Revert changes to the skinCluster influences, weightList. */
MStatus PolySkinWeightsCommand::undoCopyPolySkinWeights()
{
    MStatus status;

    MFnSkinCluster fnSkin(this->destinationSkin);

    MIntArray influenceIndices;    
    this->getInfluenceIndices(fnSkin, influenceIndices);

    status = fnSkin.setWeights(
        this->destinationMesh,
        this->destinationComponents,
        influenceIndices,
        this->oldWeightValues,
        true
    );

    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = dgModifier.undoIt();

    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;
}


/* Revert changes to the side/type/otherType attributes on the skinCluster influences. */
MStatus PolySkinWeightsCommand::undoEditPolySkinWeights()
{
    MStatus status;

    MFnSkinCluster fnSkin(this->sourceSkin);

    MDagPathArray influences;
    vector<string> influenceKeys;

    uint numberOfInfluences = fnSkin.influenceObjects(influences);
    getInfluenceKeys(fnSkin, influenceKeys);

    for (uint i = 0; i < numberOfInfluences; i++)
    {
        auto got = oldJointLabels.find(influenceKeys[i]);

        if (got != oldJointLabels.end())
        {
            setJointLabel(influences[i], oldJointLabels[influenceKeys[i]]);
        }
    }

    return MStatus::kSuccess;
}


bool PolySkinWeightsCommand::isUndoable()  const 
{ 
    return !this->isQuery; 
}