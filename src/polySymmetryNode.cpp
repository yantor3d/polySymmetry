/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "polySymmetryNode.h"

#include <string>

#include <maya/MDataBlock.h>
#include <maya/MFnData.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MIntArray.h>
#include <maya/MPlug.h>
#include <maya/MPxNode.h>
#include <maya/MObject.h>
#include <maya/MObjectHandle.h>

using namespace std; 

MObject PolySymmetryNode::numberOfEdges;
MObject PolySymmetryNode::numberOfFaces;
MObject PolySymmetryNode::numberOfVertices;

MObject PolySymmetryNode::edgeSymmetry;
MObject PolySymmetryNode::faceSymmetry;
MObject PolySymmetryNode::vertexSymmetry;

MObject PolySymmetryNode::edgeSides;
MObject PolySymmetryNode::faceSides;
MObject PolySymmetryNode::vertexSides;

PolySymmetryNode::PolySymmetryNode() {}
PolySymmetryNode::~PolySymmetryNode() {}

void* PolySymmetryNode::creator()
{
    return new PolySymmetryNode();
}


MStatus PolySymmetryNode::initialize()
{
    MStatus status;

    MFnTypedAttribute t;
    MFnNumericAttribute n;

    numberOfEdges = n.create(NUMBER_OF_EDGES, "ne", MFnNumericData::kLong, -1, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    numberOfFaces = n.create(NUMBER_OF_FACES, "nf", MFnNumericData::kLong, -1, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    numberOfVertices = n.create(NUMBER_OF_VERTICES, "nv", MFnNumericData::kLong, -1, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    edgeSymmetry = t.create(EDGE_SYMMETRY, "esy", MFnData::kIntArray, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    faceSymmetry = t.create(FACE_SYMMETRY, "fsy", MFnData::kIntArray, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    vertexSymmetry = t.create(VERTEX_SYMMETRY, "vsy", MFnData::kIntArray, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    edgeSides = t.create(EDGE_SIDES, "es", MFnData::kIntArray, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    faceSides = t.create(FACE_SIDES, "fs", MFnData::kIntArray, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    vertexSides = t.create(VERTEX_SIDES, "vs", MFnData::kIntArray, MObject::kNullObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    
    addAttribute(numberOfEdges);
    addAttribute(numberOfFaces);
    addAttribute(numberOfVertices);

    addAttribute(edgeSymmetry);
    addAttribute(faceSymmetry);
    addAttribute(vertexSymmetry);

    addAttribute(edgeSides);
    addAttribute(faceSides);
    addAttribute(vertexSides);

    return MStatus::kSuccess;    
}


MStatus PolySymmetryNode::compute(const MPlug &plug, MDataBlock &dataBlock)
{
    return MStatus::kSuccess;
}


MStatus PolySymmetryNode::setValue(MFnDependencyNode &fnNode, const char* attributeName, int &value)
{
    MStatus status;

    MPlug plug = fnNode.findPlug(attributeName, false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plug.setValue(value);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;
}


MStatus PolySymmetryNode::getValue(MFnDependencyNode &fnNode, const char* attributeName, int &value)
{
    MStatus status;

    MPlug plug = fnNode.findPlug(attributeName, true, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plug.getValue(value);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;
}


MStatus PolySymmetryNode::setValues(MFnDependencyNode &fnNode, const char* attributeName, vector<int> &values)
{
    MStatus status;

    MPlug plug = fnNode.findPlug(attributeName, false, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MIntArray valueArray;

    for (int &v : values)
    { 
        valueArray.append(v);
    }

    MFnIntArrayData valueArrayData;

    MObject data = valueArrayData.create(valueArray, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    status = plug.setMObject(data);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;
}


MStatus PolySymmetryNode::getValues(MFnDependencyNode &fnNode, const char* attributeName, vector<int> &values)
{
    MStatus status;

    MPlug plug = fnNode.findPlug(attributeName, true, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MObject data = plug.asMObject();

    MFnIntArrayData valueArrayData(data, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MIntArray valueArray = valueArrayData.array(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    uint numberOfValues = valueArray.length();
    values.resize((int) numberOfValues);

    for (uint i = 0; i < numberOfValues; i++)
    {
        values[i] = valueArray[i];
    }

    return MStatus::kSuccess;
}


MStatus PolySymmetryNode::getCacheKey(MObject &node, string &key)
{
    MStatus status;
    
    int numberOfEdges;
    int numberOfFaces;
    int numberOfVertices;

    MFnDependencyNode fnNode(node);

    PolySymmetryNode::getValue(fnNode, NUMBER_OF_EDGES, numberOfEdges);
    PolySymmetryNode::getValue(fnNode, NUMBER_OF_FACES, numberOfFaces);
    PolySymmetryNode::getValue(fnNode, NUMBER_OF_VERTICES, numberOfVertices);

    if (numberOfEdges == -1 || numberOfFaces == -1 || numberOfVertices == -1)
    {
        MString cacheErrorMessage("Cannot cache ^1s because it has no data. Use `polySymmetry -edit -cache` to cache the node once it has data.");
        cacheErrorMessage.format(cacheErrorMessage, fnNode.name());

        MGlobal::displayWarning(cacheErrorMessage);
    } else {
        key = to_string(numberOfEdges) + ":" + to_string(numberOfFaces) + ":" + to_string(numberOfVertices);
    }

    return MStatus::kSuccess;
}


MStatus PolySymmetryNode::getCacheKeyFromMesh(MDagPath &dagPath, string &key)
{
    MStatus status;
    
    MFnMesh fnMesh(dagPath);

    int numberOfEdges = fnMesh.numEdges();
    int numberOfFaces = fnMesh.numPolygons();
    int numberOfVertices = fnMesh.numVertices();

    key = to_string(numberOfEdges) + ":" + to_string(numberOfFaces) + ":" + to_string(numberOfVertices);

    return MStatus::kSuccess; 
}