#ifndef POLY_SYMMETRY_SELECTION_H
#define POLY_SYMMETRY_SELECTION_H

#include "meshData.h"

#include <vector>
#include <utility>

#include <maya/MDagPath.h>
#include <maya/MFn.h>
#include <maya/MIntArray.h>
#include <maya/MSelectionList.h>

using namespace std;

struct ComponentSelection
{
    pair<int, int> edgeIndices;
    pair<int, int> faceIndices;
    pair<int, int> vertexIndices;

    int leftVertexIndex;

    ComponentSelection() {
        leftVertexIndex = -1;

        edgeIndices = pair<int, int>(-1, -1);
        faceIndices = pair<int, int>(-1, -1);
        vertexIndices = pair<int, int>(-1, -1);
    }
};

void            getSelectedComponents(MDagPath &selectedMesh, MSelectionList &activeSelection, MSelectionList &selection, MFn::Type componentType);
void            getSelectedComponentIndices(MSelectionList &activeSelection,  vector<int> &indices, MFn::Type componentType);
bool            getSymmetricalComponentSelection(MeshData &meshData, MSelectionList &selection,  ComponentSelection &componentSelection, bool leftSideVertexSelected);

#endif