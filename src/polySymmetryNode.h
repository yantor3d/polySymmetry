/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef POLY_SYMMETRY_NODE_H
#define POLY_SYMMETRY_NODE_H

#include <string>
#include <vector>

#include <maya/MDagPath.h>
#include <maya/MDataHandle.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPxNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MString.h>
#include <maya/MTypeId.h>

#define NUMBER_OF_EDGES "numberOfEdges"
#define NUMBER_OF_FACES "numberOfFaces"
#define NUMBER_OF_VERTICES "numberOfVertices"

#define VERTEX_CHECKSUM "vertexChecksum"
#define EDGE_SYMMETRY "edgeSymmetry"
#define FACE_SYMMETRY "faceSymmetry"
#define VERTEX_SYMMETRY "vertexSymmetry"

#define EDGE_SIDES "edgeSides"
#define FACE_SIDES "faceSides"
#define VERTEX_SIDES "vertexSides"

using namespace std;

class PolySymmetryNode : MPxNode
{
public:
                        PolySymmetryNode();
    virtual            ~PolySymmetryNode();

    static  void*       creator();
    static  MStatus     initialize();
    
    virtual MStatus     compute(const MPlug &plug, MDataBlock &dataBlock);

    static MStatus      setValue(MFnDependencyNode &fnNode, const char* attributeName, int &value);
    static MStatus      getValue(MFnDependencyNode &fnNode, const char* attributeName, int &value);

    static MStatus      setValues(MFnDependencyNode &fnNode, const char* attributeName, vector<int> &values);
    static MStatus      getValues(MFnDependencyNode &fnNode, const char* attributeName, vector<int> &values);
    
    static MStatus      onInitializePlugin();
    static MStatus      onUninitializePlugin();

    static MStatus      getCacheKey(MObject &node, string &key);
    static MStatus      getCacheKeyFromMesh(MDagPath &node, string &key);

public:
    static MObject      numberOfEdges;
    static MObject      numberOfFaces;
    static MObject      numberOfVertices;

    static MObject      edgeSymmetry;
    static MObject      faceSymmetry;
    static MObject      vertexSymmetry;

    static MObject      edgeSides;
    static MObject      faceSides;
    static MObject      vertexSides;

    static MObject      vertexChecksum;
    
    static MString      NODE_NAME;
    static MTypeId      NODE_ID;
};

#endif 