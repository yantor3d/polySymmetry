#ifndef POLY_SYMMETRY_H
#define POLY_SYMMETRY_H

#include "meshData.h"
#include "selection.h"

#include <queue>
#include <utility> 
#include <vector>

#include <maya/MDagPath.h>

using namespace std;

class PolySymmetryData
{
public:
    PolySymmetryData();
    ~PolySymmetryData();

    virtual void            clear();
    virtual void            reset();
    virtual void            initialize(MDagPath &mesh);

    virtual void            findSymmetricalVertices(ComponentSelection &selection);
    virtual void            findFirstSymmetricalVertices(ComponentSelection &selection);
    virtual void            findVertexSides(vector<int> &leftSideVertexIndices);
    virtual void            finalizeSymmetry();

private:
    virtual pair<int, int>  getUnexaminedFaces(pair<int, int> &edgePair);
    virtual int             getUnexaminedFace(int &edgeIndex);
    virtual int             getUnexaminedVertexSibling(int &vertexIndex, int &faceIndex);

    virtual void            findSymmetricalVerticesOnFace(pair<int, int> &facePair);
    virtual void            findSymmetricalEdgesOnFace(queue<pair<int, int>> &symmetricalEdgesQueue, int &faceIndex);

    virtual void            markSymmetricalVertices(int &i0, int &i1);
    virtual void            markSymmetricalEdges(int &i0, int &i1);
    virtual void            markSymmetricalFaces(int &i0, int &i1);

public:
    vector<int>             vertexSymmetryIndices;
    vector<int>             edgeSymmetryIndices;
    vector<int>             faceSymmetryIndices;

    vector<int>             vertexSides;
    vector<int>             edgeSides;
    vector<int>             faceSides;
    
private:    
    MeshData                meshData;

    vector<bool>            examinedEdges;
    vector<bool>            examinedFaces;
    vector<bool>            examinedVertices;

    vector<int>             leftSideVertexIndices;
};

#endif