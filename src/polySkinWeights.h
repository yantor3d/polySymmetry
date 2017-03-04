/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef POLY_SKIN_WEIGHTS_H
#define POLY_SKIN_WEIGHTS_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPath.h>
#include <maya/MDGModifier.h>
#include <maya/MDoubleArray.h>
#include <maya/MFloatArray.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MIntArray.h>
#include <maya/MObject.h>
#include <maya/MPxCommand.h>
#include <maya/MString.h>
#include <maya/MStatus.h>
#include <maya/MSyntax.h>

using namespace std;

struct JointLabel
{
    int         side = -1;
    int         type = -1; 
    MString     otherType;

    JointLabel() {}
};

class PolySkinWeightsCommand : public MPxCommand
{
public:
                        PolySkinWeightsCommand();
    virtual             ~PolySkinWeightsCommand();

    static void*        creator();
    static MSyntax      getSyntax();

    virtual MStatus     parseArguments(MArgDatabase &argsData);
    virtual MStatus     parseEditArguments(MArgDatabase &argsData);
    virtual MStatus     parseQueryArguments(MArgDatabase &argsData);

    virtual MStatus     validateArguments();
    virtual MStatus     validateEditArguments();
    virtual MStatus     validateQueryArguments();

    virtual bool        isDeformedBy(MObject &skin, MDagPath &mesh);

    virtual MStatus     doIt(const MArgList& argList);
    virtual MStatus     redoIt();
    virtual MStatus     undoIt();

    virtual MStatus     copyPolySkinWeights();
    virtual MStatus     editPolySkinWeights();
    virtual MStatus     queryPolySkinWeights();

    virtual MStatus     undoCopyPolySkinWeights();
    virtual MStatus     undoEditPolySkinWeights();

    virtual void        copyWeightsTable(vector<string> &influenceKeys);
    virtual void        flipWeightsTable(vector<string> &influenceKeys);
    virtual void        mirrorWeightsTable(vector<string> &influenceKeys);

    virtual MStatus     makeInfluencesMatch(MFnSkinCluster &fnSourceSkin, MFnSkinCluster &fnDestinationSkin);
    virtual MStatus     makeInfluenceSymmetryTable(MDagPathArray &influences, vector<string> &influenceKeys);
    virtual bool        checkInfluenceSymmetryTable();
    
    virtual void        makeWeightTables(vector<string> &influenceKeys);
    virtual void        setWeightsTable(unordered_map<string, vector<double>> &weightTable, MDoubleArray &weights, vector<string> &influenceKeys);
    virtual void        getWeightsTable(unordered_map<string, vector<double>> &weightTable, MDoubleArray &weights, vector<string> &influenceKeys);

    virtual MStatus     getInfluenceIndices(MFnSkinCluster &fnSkin, MIntArray &influenceIndices);
    virtual MStatus     getInfluenceKeys(MFnSkinCluster &fnSkin, vector<string> &influenceKeys);

    virtual JointLabel  getJointLabel(MDagPath &influence);
    virtual MStatus     setJointLabel(MDagPath &influence, JointLabel &jointLabel);

    virtual bool        isUndoable() const;
    virtual bool        hasSyntax()  const { return true; }

public:
    static MString      COMMAND_NAME;

private:
    int                 direction     = 1;

    bool                normalizeWeights = false;
    bool                mirrorWeights = false;
    bool                flipWeights   = false;

    bool                isQuery = false;
    bool                isEdit = false;

    uint                numberOfVertices;

    string              leftInfluencePattern;
    string              rightInfluencePattern;
    
    unordered_map<string, string>         influenceSymmetry;
    unordered_map<string, JointLabel>     influenceLabels;
    unordered_map<string, vector<double>> oldWeights;
    unordered_map<string, vector<double>> newWeights;

    vector<int>         selectedVertexIndices;
    
    MDGModifier         dgModifier;
    MDoubleArray        oldWeightValues;

    MObject             sourceComponents;
    MDagPath            sourceMesh;
    MObject             sourceSkin;

    MObject             polySymmetryData;

    MObject             destinationComponents;
    MDagPath            destinationMesh;
    MObject             destinationSkin;
};

#endif 