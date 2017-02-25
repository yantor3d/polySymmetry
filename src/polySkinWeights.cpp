/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "parseArgs.h"
#include "polySkinWeights.h"
#include "polySymmetryNode.h"
#include "sceneCache.h"
#include "selection.h"

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

    if (argsData.isFlagSet(INFLUENCE_SYMMETRY_FLAG))
    {
        argsData.getFlagArgument(INFLUENCE_SYMMETRY_FLAG, 0, this->leftInfluencePattern);
        argsData.getFlagArgument(INFLUENCE_SYMMETRY_FLAG, 1, this->rightInfluencePattern);
    } else {
        MString errorMsg("^1s/^2s flag is required in edit mode.");
        errorMsg.format(errorMsg, MString(INFLUENCE_SYMMETRY_LONG_FLAG), MString(INFLUENCE_SYMMETRY_FLAG));

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

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
        MString errorMsg("The ^1s\^2s flag is required in query mode.");
        errorMsg.format(
            errorMsg, 
            MString(INFLUENCE_SYMMETRY_LONG_FLAG),
            MString(INFLUENCE_SYMMETRY_FLAG)
        );

        MGlobal::displayError(errorMsg);
        return MStatus::kFailure;
    }

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

/* Validate the command arguments */
MStatus PolySkinWeightsCommand::validateArguments()
{
    MStatus status;

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

    if (this->leftInfluencePattern.index('*') == -1 || this->rightInfluencePattern.index('*') == -1)
    {
        MString errorMsg("polySkinWeights -edit ^1s/^2s flag expects patterns with wildcards. For example: \"L_*\" \"R_*\"");
        errorMsg.format(
            errorMsg,
            MString(INFLUENCE_SYMMETRY_LONG_FLAG), 
            MString(INFLUENCE_SYMMETRY_FLAG)
        );

        MGlobal::displayError(errorMsg);

        return MStatus::kFailure;
    }

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
    this->makeInfluenceSymmetryTable(influences, influenceKeys);

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
   
    // TODO: replace with actual C++ code.
    
    status = MGlobal::executePythonCommand("import fnmatch");
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MString leftTokenCmd("'^1s'.replace('*', '').replace('$', '')");
    leftTokenCmd.format(leftTokenCmd, leftInfluencePattern);

    MString rightTokenCmd("'^1s'.replace('*', '').replace('$', '')");
    rightTokenCmd.format(rightTokenCmd, rightInfluencePattern);

    MString leftToken;
    MString rightToken;

    status = MGlobal::executePythonCommand(leftTokenCmd, leftToken);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = MGlobal::executePythonCommand(rightTokenCmd, rightToken);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    for (uint i = 0; i < numberOfInfluences; i++)
    {
        MString influenceName = influences[i].partialPathName();

        if (!influences[i].hasFn(MFn::kJoint))
        {
            MGlobal::displayWarning("Cannot set " + influenceName + " .side, .type, and .otherType attributes because it is not a joint.");
            continue;
        }

        int leftMatch = 0;
        int rightMatch = 0;

        MString leftMatchCmd("fnmatch.fnmatch('^1s', '^2s')");
        leftMatchCmd.format(leftMatchCmd, influenceName, leftInfluencePattern);

        MString rightMatchCmd("fnmatch.fnmatch('^1s', '^2s')");
        rightMatchCmd.format(rightMatchCmd, influenceName, rightInfluencePattern);

        status = MGlobal::executePythonCommand(leftMatchCmd, leftMatch);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        status = MGlobal::executePythonCommand(rightMatchCmd, rightMatch);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        MString replaceTokenCmd("'^1s'.replace('^2s', '')");
        
        if (leftMatch == 1)
        {
            replaceTokenCmd.format(replaceTokenCmd, influenceName, leftToken);
            status = MGlobal::executePythonCommand(replaceTokenCmd, influenceName);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        } else if (rightMatch == 1) {
            replaceTokenCmd.format(replaceTokenCmd, influenceName, rightToken);
            status = MGlobal::executePythonCommand(replaceTokenCmd, influenceName);
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }

        JointLabel oldJointLabel = getJointLabel(influences[i]);
        influenceLabels.emplace(influenceKeys[i], oldJointLabel);

        JointLabel newJointLabel;

        newJointLabel.type = OTHER_TYPE;
        newJointLabel.otherType = influenceName;

        if (leftMatch == 1)
        { 
            newJointLabel.side = LEFT_SIDE;
        } else if (rightMatch == 1) {
            newJointLabel.side = RIGHT_SIDE;
        } else {
            newJointLabel.side = CENTER_SIDE;
        }
        
        setJointLabel(influences[i], newJointLabel);
    }

    this->makeInfluenceSymmetryTable(influences, influenceKeys);
    bool symmetryIsSet = this->checkInfluenceSymmetryTable();

    if (!symmetryIsSet)
    {
        MString warning("Influences for '^1s' were not configured. Check your arguments for the ^2s/^3s flag and try again..");
        warning.format(
            warning, 
            fnSkin.name(), 
            MString(INFLUENCE_SYMMETRY_LONG_FLAG),
            MString(INFLUENCE_SYMMETRY_FLAG)
        );

        MGlobal::displayWarning(warning);
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

    this->numberOfVertices = MFnMesh(sourceMesh).numVertices();

    numSourceInfluences = fnSourceSkin.influenceObjects(sourceInfluences);
    this->getInfluenceIndices(fnSourceSkin, sourceInfluenceIndices);
    this->getInfluenceKeys(fnSourceSkin, sourceInfluenceKeys);
    getAllComponents(sourceMesh, sourceComponents);

    numDestinationInfluences = fnDestinationSkin.influenceObjects(destinationInfluences);
    this->getInfluenceIndices(fnDestinationSkin, destinationInfluenceIndices);
    this->getInfluenceKeys(fnDestinationSkin, destinationInfluenceKeys);
    getAllComponents(destinationMesh, destinationComponents);

    this->makeInfluenceSymmetryTable(destinationInfluences, destinationInfluenceKeys);

    bool symmetryIsSet = this->checkInfluenceSymmetryTable();

    if (!symmetryIsSet)
    {
        MString verboseWarning(
            "All joint influence's have the the same side/type/otherType attribute values. "
            "Run 'polySymmetry -edit -^1s \"[LEFT PATTERN]\" \"[RIGHT PATTERN]\"' to auto-configure the influences."
        );

        MString warning("Influences for '^1s' are not configured. See script editor for details.");

        verboseWarning.format(verboseWarning, MString(INFLUENCE_SYMMETRY_LONG_FLAG));
        warning.format(warning, fnDestinationSkin.name());

        MGlobal::displayWarning(verboseWarning);
        MGlobal::displayWarning(warning);
    }
    
    this->makeWeightTables(destinationInfluenceKeys);

    status = fnSourceSkin.getWeights(sourceMesh, sourceComponents, sourceInfluenceIndices, sourceWeights);
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
MStatus PolySkinWeightsCommand::makeInfluenceSymmetryTable(MDagPathArray &influences, vector<string> &influenceKeys)
{
    MStatus status;

    uint numberOfInfluences = influences.length();

    unordered_map<string, JointLabel> jointLabels;
   
    for (uint i = 0; i < numberOfInfluences; i++)
    {
        JointLabel lbl = this->getJointLabel(influences[i]);
        string key = influenceKeys[i];
        jointLabels.emplace(key, lbl);

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

/* Confirm that at least one pair of influences are symmetrical to each other. */
bool PolySkinWeightsCommand::checkInfluenceSymmetryTable()
{
    bool result = false;

    for (auto it = this->influenceSymmetry.begin(); it != this->influenceSymmetry.end(); ++it)
    {
        if (it->first != it->second)
        {
            result = true;
        }
    }

    return result;
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

    for (uint i = 0; i < this->numberOfVertices; i++)
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
        for (uint i = 0; i < numberOfVertices; i++)
        {
            int o = vertexSymmetry[i];
            newWeights[ii][i] = oldWeights[ii][o];
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
        for (uint i = 0; i < numberOfVertices; i++)
        {
            if (newWeights.find(ii) == newWeights.end())
            {
                continue;
            }

            if (vertexSides[i] == 0)
            {
                string oi = influenceSymmetry[ii];

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
                int o = vertexSymmetry[i];
                string oi = influenceSymmetry[ii];

                if (oldWeights.find(oi) != oldWeights.end()) 
                {
                    newWeights[ii][i] = oldWeights[oi][o];
                } else {
                    newWeights[ii][i] = 0.0;
                }
            }
        }
    }

    for (uint i = 0; i < numberOfVertices; i++)
    {
        double centerWeightMax = 0.0;
        double centerWeights = 0.0;
        double sideWeights = 0.0;

        for (string ii : influenceKeys)
        {
            string oi = influenceSymmetry[ii];

            double wt = newWeights[ii][i];

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
        auto got = influenceLabels.find(influenceKeys[i]);

        if (got != influenceLabels.end())
        {
            setJointLabel(influences[i], influenceLabels[influenceKeys[i]]);
        }
    }

    return MStatus::kSuccess;
}

bool PolySkinWeightsCommand::isUndoable()  const 
{ 
    return !this->isQuery; 
}