/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#ifndef POLY_SYMMETRY_SCENE_CACHE_H
#define POLY_SYMMETRY_SCENE_CACHE_H

#include <string>
#include <unordered_map>

#include <maya/MCallbackIdArray.h>
#include <maya/MObject.h>
#include <maya/MObjectHandle.h>
#include <maya/MStatus.h>

using namespace std; 

class PolySymmetryCache
{
public:
    static MStatus      initialize();
    static MStatus      uninitialize();

    static void         sceneUpdateCallback(void* clientData);

    static void         nodeAddedCallback(MObject &node, void* clientData);
    static void         nodeRemovedCallback(MObject &node, void* clientData);

    static void         newFileCallback(void* clientData);

    static void         beforeOpenFileCallback(void* clientData);
    static void         afterOpenFileCallback(void* clientData);

    static void         addNodeToCache(MObject &node);
    static bool         getNodeFromCache(MDagPath &mesh, MObject &node);

public:
    static unordered_map<string, MObjectHandle>     symmetryNodeCache;

    static MCallbackIdArray     callbackIDs;
    static bool                 openingFile;
};

#endif