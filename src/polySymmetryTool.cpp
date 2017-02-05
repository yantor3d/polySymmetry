/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "polySymmetryTool.h"
#include "polySymmetryCmd.h"
#include "selection.h"
#include "sceneCache.h"
#include "util.h"

#include <algorithm>
#include <sstream>
#include <vector>

#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MItSelectionList.h>
#include <maya/MPlug.h>
#include <maya/MPxContext.h>
#include <maya/MPxToolCommand.h>
#include <maya/MSelectionList.h>
#include <maya/MString.h>
#include <maya/MStatus.h>

using namespace std;

PolySymmetryTool::PolySymmetryTool() {}

PolySymmetryTool::~PolySymmetryTool() 
{
    selectedComponents.clear();
    leftSideVertexIndices.clear();
}

void PolySymmetryTool::toolOnSetup(MEvent &event) 
{
    meshData = MeshData();

    this->getSelectedMesh();
    this->updateHelpString();
}

void PolySymmetryTool::toolOffCleanup()
{

}

MStatus PolySymmetryTool::helpStateHasChanged(MEvent &event)
{
    this->updateHelpString();

    return MStatus::kSuccess;
}

void PolySymmetryTool::deleteAction()
{
    if (selectedComponents.empty())
    {
        this->clearSelectedMesh();
    } else {
        selectedComponents.pop_back();
        leftSideVertexIndices.pop_back();
    }

    if (selectedMesh.isValid())
    {
        this->symmetryData.reset();
    } else {
        this->symmetryData.clear();
    }

    this->updateHelpString();
    this->recalculateSymmetry();
    this->updateDisplayColors();
}

void PolySymmetryTool::abortAction()
{
    MGlobal::executeCommand("escapeCurrentTool");
}

void PolySymmetryTool::completeAction() 
{
    bool continueSelecting = true;

    if (!selectedMesh.isValid())
    {
        this->getSelectedMesh();      
    } else {    
        MSelectionList activeSelection;
        MGlobal::getActiveSelectionList(activeSelection);      

        if (activeSelection.isEmpty())
        {
            continueSelecting = false;
        } else {
            ComponentSelection s;

            MSelectionList selection;

            getSelectedComponents(selectedMesh, activeSelection, selection, MFn::Type::kMeshEdgeComponent);
            getSelectedComponents(selectedMesh, activeSelection, selection, MFn::Type::kMeshVertComponent);
            getSelectedComponents(selectedMesh, activeSelection, selection, MFn::Type::kMeshPolygonComponent);

            bool isValidSelection = getSymmetricalComponentSelection(meshData, selection, s, true);

            if (isValidSelection)
            {
                this->selectedComponents.push_back(s);
                this->leftSideVertexIndices.push_back(s.leftVertexIndex);
                MGlobal::clearSelectionList();  
            } else {
                MGlobal::displayError("Must select a symmetrical edge, face, and vertex on both sides of the mesh, and a lone vertex on the left side of the mesh.");
                this->updateHelpString();
                return;
            }
        }
    }

    if (continueSelecting)
    {
        this->updateHelpString();
        this->recalculateSymmetry();
        this->updateDisplayColors();
    } else {
        bool selectionIsComplete = true;

        if (!selectedMesh.isValid())
        {
            MGlobal::displayError("Select a mesh to calculate the symmetry of.");
            selectionIsComplete = false;
        }

        if (selectedComponents.empty())
        {
            MGlobal::displayError("Must select at least set of symmetrical components.");
            selectionIsComplete = false;
        }
    
        if (selectionIsComplete)
        {
            PolySymmetryCommand *cmd = (PolySymmetryCommand*) newToolCommand();
            (*cmd).finalize();

            doToolCommand();
            clearSelectedMesh();
        }
    }
}

void PolySymmetryTool::doToolCommand()
{
    MGlobal::executeCommand("escapeCurrentTool");

    stringstream ss;
    ss << PolySymmetryCommand::COMMAND_NAME << ' ';
    
    for (ComponentSelection &s : selectedComponents)
    {
        ss << SYMMETRY_COMPONENTS_FLAG << ' ';

        ss << selectedMesh.partialPathName() << ".f[" << s.faceIndices.first << "] ";
        ss << selectedMesh.partialPathName() << ".f[" << s.faceIndices.second << "] ";

        ss << selectedMesh.partialPathName() << ".e[" << s.edgeIndices.first << "] ";
        ss << selectedMesh.partialPathName() << ".e[" << s.edgeIndices.second << "] ";

        ss << selectedMesh.partialPathName() << ".vtx[" << s.vertexIndices.first << "] ";
        ss << selectedMesh.partialPathName() << ".vtx[" << s.vertexIndices.second << "] ";

        ss << LEFT_SIDE_VERTEX_FLAG << ' ';

        ss << selectedMesh.partialPathName() << ".vtx[" << s.leftVertexIndex << "] ";
    }

    ss << selectedMesh.partialPathName() << ';';

    MGlobal::executeCommand(ss.str().c_str(), true);
}

MStatus PolySymmetryTool::getSelectedMesh()
{
    MStatus status;

    MSelectionList activeSelection;
    MGlobal::getActiveSelectionList(activeSelection);

    if (!activeSelection.isEmpty())
    {
        MItSelectionList iterSelection(activeSelection);
        MDagPath object;

        while (!iterSelection.isDone())
        {
            iterSelection.getDagPath(object);

            if (object.hasFn(MFn::kMesh))
            {
                this->selectedMesh.set(object);
                break;
            }
        }
    }

    if (this->selectedMesh.isValid())
    {
        MObject obj;
        bool cacheHit = PolySymmetryCache::getNodeFromCache(this->selectedMesh, obj);

        if (cacheHit)
        {
            MString warningMsg("Symmetry data already exists for ^1s - ^2s.");
            warningMsg.format(warningMsg, selectedMesh.partialPathName(), MFnDependencyNode(obj).name());

            MGlobal::displayError(warningMsg);

            this->abortAction();
            return MStatus::kFailure;
        }

        symmetryData.initialize(selectedMesh);
        meshData.unpackMesh(selectedMesh);
        MFnMesh meshFn(selectedMesh);

        status = meshFn.getVertexColors(originalVertexColors);
        hasVertexColors = status == MStatus::kSuccess;

        meshFn.clearColors();

        MPlug displayColorsPlug = meshFn.findPlug("displayColors");

        originalDisplayColors = displayColorsPlug.asBool();
        status = displayColorsPlug.setBool(true);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        if (selectedMesh.node().hasFn(MFn::kMesh))
        {
            selectedMesh.pop();
        }
    }

    return MStatus::kSuccess;
}

MStatus PolySymmetryTool::clearSelectedMesh()
{
    MStatus status;

    if (selectedMesh.isValid())
    {
        MFnMesh meshFn(selectedMesh);

        if (hasVertexColors)
        {
            MIntArray vertexList(meshData.numberOfVertices);
            for (int i = 0; i < meshData.numberOfVertices; i++) { vertexList.set(i, i); }

            status = meshFn.setVertexColors(originalVertexColors, vertexList);;
            CHECK_MSTATUS_AND_RETURN_IT(status);
        }
        
        status = meshFn.findPlug("displayColors").setBool(originalDisplayColors);
        CHECK_MSTATUS_AND_RETURN_IT(status);

        selectedMesh.set(MDagPath());
        meshData.clear();
    }    

    return MStatus::kSuccess;

}

void PolySymmetryTool::updateHelpString()
{
    if (selectedMesh.isValid())
    {
        MDagPath parent = MDagPath(selectedMesh);
        parent.pop();

        this->setHelpString("Select components on " + parent.partialPathName() + " that define the symmetry and press ENTER.");
    } else {
        this->setHelpString("Select a mesh to find the symmetry on and press ENTER");
    }
}

void PolySymmetryTool::recalculateSymmetry()
{
    if (!selectedMesh.isValid()) { return; }

    for (ComponentSelection &s : selectedComponents)
    {
        this->symmetryData.findSymmetricalVertices(s);
    }

    this->symmetryData.findVertexSides(this->leftSideVertexIndices);
}

void PolySymmetryTool::updateDisplayColors()
{
    if (!selectedMesh.isValid()) { return; }

    MIntArray vertexList(meshData.numberOfVertices);
    MColorArray colors(meshData.numberOfVertices);

    for (int i = 0; i < meshData.numberOfVertices; i++)
    {
        float r = 0.5f;
        float g = 0.5f;
        float b = 0.5f;

        if (this->symmetryData.vertexSides[i] == 1)
        {
            b = 0.75f;
        } else if (this->symmetryData.vertexSides[i] == -1) {
            r = 0.75f;
        }

        vertexList[i] = i;
        colors[i] = MColor(r, g, b);
    }

    MFnMesh meshFn(selectedMesh);
    meshFn.setVertexColors(colors, vertexList);
}

PolySymmetryContextCmd::PolySymmetryContextCmd() {}
PolySymmetryContextCmd::~PolySymmetryContextCmd() {}

MPxContext* PolySymmetryContextCmd::makeObj()
{
    return new PolySymmetryTool();
}

void* PolySymmetryContextCmd::creator()
{
    return new PolySymmetryContextCmd();
}