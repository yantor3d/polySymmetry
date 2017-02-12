#ifndef PARSE_ARGS_H
#define PARSE_ARGS_H

#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MFn.h>
#include <maya/MObject.h>
#include <maya/MStatus.h>

namespace parseArgs
{
    MStatus getNodeArgument(MArgDatabase &argsData, const char* flag, MObject &node, bool required);
    MStatus getDagPathArgument(MArgDatabase &argsData, const char* flag, MDagPath &path, bool required);

    bool isNodeType(MObject &node, MFn::Type nodeType);
    bool isNodeType(MDagPath &path, MFn::Type nodeType);
}

#endif