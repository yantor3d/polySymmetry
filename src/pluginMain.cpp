/**
    Copyright (c) 2017 Ryan Porter
*/

/*
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include "polySymmetryTool.h"
#include "polySymmetryCmd.h"
#include "polySymmetryNode.h"

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

    return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin fnPlugin(obj, kAUTHOR, kVERSION, kREQUIRED_API_VERSION);

    fnPlugin.deregisterContextCommand(
        PolySymmetryContextCmd::COMMAND_NAME, 
        PolySymmetryCommand::COMMAND_NAME
    );

    fnPlugin.deregisterNode(PolySymmetryNode::NODE_ID);

    return MS::kSuccess;
}