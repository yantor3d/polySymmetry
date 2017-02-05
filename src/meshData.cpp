/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include "meshData.h"
#include "util.h"

#include <algorithm>
#include <vector>
#include <unordered_map>

#include <maya/MDagPath.h>
#include <maya/MIntArray.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>

using namespace std;

MeshData::MeshData() {}

MeshData::~MeshData()
{
    this->clear();
}

void MeshData::clear()
{
    numberOfEdges = 0;
    numberOfFaces = 0;
    numberOfVertices = 0;

    vertexData.clear();
    edgeData.clear();
    faceData.clear();  
}

void MeshData::unpackMesh(MDagPath &meshDagPath)
{
    this->clear();
    
    MItMeshEdge edges(meshDagPath);
    MItMeshPolygon faces(meshDagPath);
    MItMeshVertex vertices(meshDagPath);

    this->unpackEdges(edges);
    this->unpackFaces(faces);
    this->unpackVertices(vertices);

    this->unpackVertexSiblings();
}

void MeshData::unpackEdges(MItMeshEdge &edges)
{
    this->numberOfEdges = edges.count();
    this->edgeData.resize(this->numberOfEdges);

    MIntArray connectedEdges;
    MIntArray connectedFaces;

    edges.reset();

    while (!edges.isDone())
    {
        EdgeData &edge = edgeData[edges.index()];

        edge.connectedVertices.resize(2);
        edge.connectedVertices[0] = edges.index(0);
        edge.connectedVertices[1] = edges.index(1);

        sort(edge.connectedVertices.begin(), edge.connectedVertices.end());
        
        edges.getConnectedFaces(connectedFaces);
        edges.getConnectedEdges(connectedEdges);

        insertAll(connectedFaces, edge.connectedFaces);
        insertAll(connectedEdges, edge.connectedEdges);

        edges.next();
    }
}

void MeshData::unpackFaces(MItMeshPolygon &faces)
{
    this->numberOfFaces = faces.count();
    this->faceData.resize(this->numberOfFaces);

    MIntArray connectedEdges;
    MIntArray connectedFaces;
    MIntArray connectedVertices;

    faces.reset();

    while (!faces.isDone())
    {
        FaceData &face = faceData[faces.index()];

        faces.getEdges(connectedEdges);
        faces.getConnectedFaces(connectedFaces);
        faces.getVertices(connectedVertices);

        insertAll(connectedFaces, face.connectedFaces);
        insertAll(connectedEdges, face.connectedEdges);
        insertAll(connectedVertices, face.connectedVertices);

        faces.next();
    }
}

void MeshData::unpackVertices(MItMeshVertex &vertices)
{
    this->numberOfVertices = vertices.count();
    this->vertexData.resize(this->numberOfVertices);

    MIntArray connectedEdges;
    MIntArray connectedFaces;
    MIntArray connectedVertices;

    vertices.reset();

    while (!vertices.isDone())
    {
        VertexData &vertex = vertexData[vertices.index()];

        vertices.getConnectedEdges(connectedEdges);
        vertices.getConnectedFaces(connectedFaces);
        vertices.getConnectedVertices(connectedVertices);

        insertAll(connectedFaces, vertex.connectedFaces);
        insertAll(connectedEdges, vertex.connectedEdges);
        insertAll(connectedVertices, vertex.connectedVertices);

        vertices.next();
    }
}

void MeshData::unpackVertexSiblings()
{
    for (int vertexIndex = 0; vertexIndex < this->numberOfVertices; vertexIndex++)
    {
        for (int &faceIndex : vertexData[vertexIndex].connectedFaces)
        {
            vertexData[vertexIndex].faceVertexSiblings.emplace(faceIndex, vector<int>());

            for (int &faceVertexIndex : faceData[faceIndex].connectedVertices)
            {
                if (contains(vertexData[faceVertexIndex].connectedVertices, vertexIndex))
                {
                    vertexData[vertexIndex].faceVertexSiblings[faceIndex].push_back(faceVertexIndex);
                }
            }

            sort(
                vertexData[vertexIndex].faceVertexSiblings[faceIndex].begin(), 
                vertexData[vertexIndex].faceVertexSiblings[faceIndex].end()
            );
        }
    }
}

void MeshData::insertAll(MIntArray &src, vector<int> &dest)
{
    dest.resize(src.length());

    for (uint i = 0; i < src.length(); i++)
    {
        dest[i] = src[i];
    }

    sort(dest.begin(), dest.end());
}
