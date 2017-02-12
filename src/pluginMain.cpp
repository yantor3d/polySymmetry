/**
    Copyright (c) 2017 Ryan Porter
*/

#include "polyDeformerWeights.h"
#include "polyFlipCmd.h"
#include "polyMirrorCmd.h"
#include "polySkinWeights.h"
#include "polySymmetryTool.h"
#include "polySymmetryCmd.h"
#include "polySymmetryNode.h"
#include "sceneCache.h"

#include <maya/MFnPlugin.h>
#include <maya/MTypeId.h>
#include <maya/MString.h>
#include <maya/MStatus.h>

const char* kAUTHOR = "Ryan Porter";
const char* kVERSION = "0.1.0";
const char* kREQUIRED_API_VERSION = "Any";

MString PolyDeformerWeightsCommand::COMMAND_NAME    = "polyDeformerWeights";
MString PolyFlipCommand::COMMAND_NAME               = "polyFlip";
MString PolyMirrorCommand::COMMAND_NAME             = "polyMirror";
MString PolySkinWeightsCommand::COMMAND_NAME        = "polySkinWeights";

MString PolySymmetryContextCmd::COMMAND_NAME        = "polySymmetryCtx";
MString PolySymmetryCommand::COMMAND_NAME           = "polySymmetry";

MString PolySymmetryNode::NODE_NAME                 = "polySymmetryData";
MTypeId PolySymmetryNode::NODE_ID                   = 0x00126b0d;

#define REGISTER_COMMAND(CMD) CHECK_MSTATUS_AND_RETURN_IT(fnPlugin.registerCommand(CMD::COMMAND_NAME, CMD::creator, CMD::getSyntax));
#define DEREGISTER_COMMAND(CMD) CHECK_MSTATUS_AND_RETURN_IT(fnPlugin.deregisterCommand(CMD::COMMAND_NAME))

MStatus initializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin fnPlugin(obj, kAUTHOR, kVERSION, kREQUIRED_API_VERSION);

    status = fnPlugin.registerContextCommand(
        PolySymmetryContextCmd::COMMAND_NAME,
        PolySymmetryContextCmd::creator,
        PolySymmetryCommand::COMMAND_NAME,
        PolySymmetryCommand::creator,
        PolySymmetryCommand::getSyntax
    );

    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.registerNode(
	    PolySymmetryNode::NODE_NAME,
        PolySymmetryNode::NODE_ID,
        PolySymmetryNode::creator,
        PolySymmetryNode::initialize,
		MPxNode::kDependNode
    );

    CHECK_MSTATUS_AND_RETURN_IT(status);

    REGISTER_COMMAND(PolyDeformerWeightsCommand);
    REGISTER_COMMAND(PolyFlipCommand);
    REGISTER_COMMAND(PolyMirrorCommand);
    REGISTER_COMMAND(PolySkinWeightsCommand);

    status = PolySymmetryCache::initialize();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin fnPlugin(obj, kAUTHOR, kVERSION, kREQUIRED_API_VERSION);

    status = PolySymmetryCache::uninitialize();
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = fnPlugin.deregisterContextCommand(
        PolySymmetryContextCmd::COMMAND_NAME, 
        PolySymmetryCommand::COMMAND_NAME
    );

    CHECK_MSTATUS_AND_RETURN_IT(status);

    DEREGISTER_COMMAND(PolyDeformerWeightsCommand);
    DEREGISTER_COMMAND(PolyFlipCommand);
    DEREGISTER_COMMAND(PolyMirrorCommand);
    DEREGISTER_COMMAND(PolySkinWeightsCommand);

    status = fnPlugin.deregisterNode(PolySymmetryNode::NODE_ID);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MS::kSuccess;
}