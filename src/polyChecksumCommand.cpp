/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "meshData.h"
#include "polyChecksumCommand.h"

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MItMeshVertex.h>
#include <maya/MIntArray.h>
#include <maya/MPxCommand.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>

using namespace std;

PolyChecksumCommand::PolyChecksumCommand()  {}
PolyChecksumCommand::~PolyChecksumCommand() {}

void* PolyChecksumCommand::creator()
{
    return new PolyChecksumCommand();
}

MSyntax PolyChecksumCommand::getSyntax()
{
    MSyntax syntax;

    syntax.setObjectType(MSyntax::kSelectionList, 1, 1);
    syntax.useSelectionAsDefault(true);

    syntax.enableQuery(false);
    syntax.enableEdit(false);

    return syntax;
}

MStatus PolyChecksumCommand::doIt(const MArgList& argList)
{
    MStatus status;
    MArgDatabase argsData(syntax(), argList);

    MSelectionList selection;
    argsData.getObjects(selection);

    status = selection.getDagPath(0, this->mesh);

    if (!status || !mesh.hasFn(MFn::Type::kMesh))
    {
        MGlobal::displayError("Must select a mesh.");
        return MStatus::kFailure;
    }

    return this->redoIt();
}

MStatus PolyChecksumCommand::redoIt()
{
    unsigned long checksum = MeshData::getVertexChecksum(this->mesh);
	this->setResult((int) checksum);

    return MStatus::kSuccess;
}