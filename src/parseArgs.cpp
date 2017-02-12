/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "parseArgs.h"

#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

MStatus parseArgs::getNodeArgument(MArgDatabase &argsData, const char* flag, MObject &node, bool required)
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

MStatus parseArgs::getDagPathArgument(MArgDatabase &argsData, const char* flag, MDagPath &path, bool required)
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

bool parseArgs::isNodeType(MObject &node, MFn::Type nodeType)
{
    return !node.isNull() && node.hasFn(nodeType);
}

bool parseArgs::isNodeType(MDagPath &path, MFn::Type nodeType)
{
    return path.isValid() && path.hasFn(nodeType);
}