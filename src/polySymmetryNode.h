#ifndef POLY_SYMMETRY_NODE_H
#define POLY_SYMMETRY_NODE_H

#include <maya/MDataHandle.h>
#include <maya/MPxNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

#define EDGE_SYMMETRY "edgeSymmetry"
#define FACE_SYMMETRY "faceSymmetry"
#define VERTEX_SYMMETRY "vertexSymmetry"

#define EDGE_SIDES "edgeSides"
#define FACE_SIDES "faceSides"
#define VERTEX_SIDES "vertexSides"

class PolySymmetryNode : MPxNode
{
public:
                        PolySymmetryNode();
    virtual            ~PolySymmetryNode();

    virtual MStatus     compute(const MPlug &plug, MDataBlock &dataBlock);
    static  MStatus     initialize();

    static  void*       creator();
    
public:
    static MObject      edgeSymmetry;
    static MObject      faceSymmetry;
    static MObject      vertexSymmetry;

    static MObject      edgeSides;
    static MObject      faceSides;
    static MObject      vertexSides;

    static  MString     NODE_NAME;
    static  MTypeId     NODE_ID;
};

#endif 