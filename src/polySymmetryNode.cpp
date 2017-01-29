#include "polySymmetryNode.h"

#include <maya/MDataBlock.h>
#include <maya/MFnData.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MPxNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>

MObject PolySymmetryNode::edgeSymmetry;
MObject PolySymmetryNode::faceSymmetry;
MObject PolySymmetryNode::vertexSymmetry;

MObject PolySymmetryNode::edgeSides;
MObject PolySymmetryNode::faceSides;
MObject PolySymmetryNode::vertexSides;

PolySymmetryNode::PolySymmetryNode() {}
PolySymmetryNode::~PolySymmetryNode() {}

MStatus PolySymmetryNode::compute(const MPlug &plug, MDataBlock &dataBlock)
{
    return MStatus::kSuccess;
}

MStatus PolySymmetryNode::initialize()
{
    MStatus status;

    MFnTypedAttribute t;

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
    
    addAttribute(edgeSymmetry);
    addAttribute(faceSymmetry);
    addAttribute(vertexSymmetry);

    addAttribute(edgeSides);
    addAttribute(faceSides);
    addAttribute(vertexSides);

    return MStatus::kSuccess;    
}

void* PolySymmetryNode::creator()
{
    return new PolySymmetryNode();
}