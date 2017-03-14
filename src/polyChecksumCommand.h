/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef TOPOLOGY_CHECKSUM_COMMAND_H
#define TOPOLOGY_CHECKSUM_COMMAND_H

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MPxCommand.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>

class PolyChecksumCommand : public MPxCommand
{
public:
                        PolyChecksumCommand();
    virtual             ~PolyChecksumCommand();

    static void*        creator();
    static MSyntax      getSyntax();

    virtual MStatus     doIt(const MArgList& argList);
    virtual MStatus     redoIt();

    virtual bool        isUndoable() const { return false; }
    virtual bool        hasSyntax()  const { return true; }
    
public:
    static MString      COMMAND_NAME;

private:    
    MDagPath            mesh;
};

#endif