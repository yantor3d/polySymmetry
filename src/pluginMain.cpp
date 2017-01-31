/**
    Copyright (c) 2017 Ryan Porter
*/

#include "polySymmetryTool.h"
#include "polySymmetryCmd.h"
#include "polySymmetryNode.h"
#include "sceneCache.h"

#include <maya/MFnPlugin.h>
#include <maya/MTypeId.h>
#include <maya/MString.h>
#include <maya/MStatus.h>

const char* kAUTHOR = "Ryan Porter";
const char* kVERSION = "0.0.1";
const char* kREQUIRED_API_VERSION = "Any";

MString PolySymmetryContextCmd::COMMAND_NAME = "polySymmetryCtx";
MString PolySymmetryCommand::COMMAND_NAME = "polySymmetry";

MString PolySymmetryNode::NODE_NAME = "polySymmetryData";
MTypeId PolySymmetryNode::NODE_ID = 0x00126b0d;

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

    fnPlugin.deregisterContextCommand(
        PolySymmetryContextCmd::COMMAND_NAME, 
        PolySymmetryCommand::COMMAND_NAME
    );

    fnPlugin.deregisterNode(PolySymmetryNode::NODE_ID);

    return MS::kSuccess;
}