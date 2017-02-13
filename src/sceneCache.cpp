/**
    Copyright (c) 2017 Ryan Porter    
    You may use, distribute, or modify this code under the terms of the MIT license.
*/

#include <string>
#include <unordered_map>
#include <utility>

#include "polySymmetryNode.h"
#include "sceneCache.h"

#include <maya/MCallbackIdArray.h>
#include <maya/MDGMessage.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MMessage.h>
#include <maya/MObject.h>
#include <maya/MObjectHandle.h>
#include <maya/MSceneMessage.h>
#include <maya/MStatus.h>


unordered_map<string, MObjectHandle>    PolySymmetryCache::symmetryNodeCache;
MCallbackIdArray                        PolySymmetryCache::callbackIDs;
bool                                    PolySymmetryCache::cacheNodes;

MStatus PolySymmetryCache::initialize() 
{
    MStatus status;

    PolySymmetryCache::cacheNodes = true;
    PolySymmetryCache::symmetryNodeCache = unordered_map<string, MObjectHandle>();
    PolySymmetryCache::callbackIDs = MCallbackIdArray();

    MCallbackId sceneUpdateCallbackId = MSceneMessage::addCallback(MSceneMessage::kSceneUpdate, PolySymmetryCache::sceneUpdateCallback, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MCallbackId afterNewCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterNew, PolySymmetryCache::newFileCallback, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MCallbackId beforeOpenCallbackId = MSceneMessage::addCallback(MSceneMessage::kBeforeOpen, PolySymmetryCache::beforeOpenFileCallback, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MCallbackId afterOpenCallbackId = MSceneMessage::addCallback(MSceneMessage::kAfterOpen, PolySymmetryCache::afterOpenFileCallback, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MCallbackId nodeAddedCallbackId = MDGMessage::addNodeAddedCallback(PolySymmetryCache::nodeAddedCallback, PolySymmetryNode::NODE_NAME, NULL, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    MCallbackId nodeRemovedCallbackId = MDGMessage::addNodeRemovedCallback(PolySymmetryCache::nodeRemovedCallback, PolySymmetryNode::NODE_NAME, NULL, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    callbackIDs.append(sceneUpdateCallbackId);
    callbackIDs.append(afterNewCallbackId);
    callbackIDs.append(beforeOpenCallbackId);
    callbackIDs.append(afterOpenCallbackId);
    callbackIDs.append(nodeAddedCallbackId);
    callbackIDs.append(nodeRemovedCallbackId);

    return MStatus::kSuccess;
}

MStatus PolySymmetryCache::uninitialize() 
{
    MStatus status;

    status = MMessage::removeCallbacks(callbackIDs);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return MStatus::kSuccess;
}
    
void PolySymmetryCache::sceneUpdateCallback(void* clientData) 
{
    if (PolySymmetryCache::cacheNodes)
    {
        PolySymmetryCache::cacheNodes = false;
    }
}

void PolySymmetryCache::nodeAddedCallback(MObject &node, void* clientData)
{
    if (PolySymmetryCache::cacheNodes)
    {
        PolySymmetryCache::addNodeToCache(node);
    }
}

void PolySymmetryCache::nodeRemovedCallback(MObject &node, void* clientData)
{
    if (!PolySymmetryCache::cacheNodes) { return; }

    string key;
    PolySymmetryNode::getCacheKey(node, key);

    if (!key.empty() && PolySymmetryCache::symmetryNodeCache.count(key) > 0)
    {
        PolySymmetryCache::symmetryNodeCache.erase(key);
    }
}

void PolySymmetryCache::newFileCallback(void* clientData)
{
    PolySymmetryCache::symmetryNodeCache.clear();
}

void PolySymmetryCache::beforeOpenFileCallback(void* clientData)
{
    PolySymmetryCache::cacheNodes = false;
}

void PolySymmetryCache::afterOpenFileCallback(void* clientData)
{
    PolySymmetryCache::cacheNodes = true;

    PolySymmetryCache::symmetryNodeCache.clear();

    MItDependencyNodes itNodes;
    MFnDependencyNode fnNode;
    MObject node;

    while (!itNodes.isDone())
    {
        node = itNodes.thisNode();

        fnNode.setObject(node);

        if (fnNode.typeName() == PolySymmetryNode::NODE_NAME)
        {
            PolySymmetryCache::addNodeToCache(node);
        }

        itNodes.next();
    }   
}

void PolySymmetryCache::addNodeToCache(MObject &node)
{
    string key;
    PolySymmetryNode::getCacheKey(node, key);

    if (!key.empty())
    {
        MObjectHandle handle(node);
        PolySymmetryCache::symmetryNodeCache.emplace(key, handle);
    }
}

bool PolySymmetryCache::getNodeFromCache(MDagPath &mesh, MObject &node)
{
    bool result = false;

    string key;
    PolySymmetryNode::getCacheKeyFromMesh(mesh, key);

    if (!key.empty())
    {
        auto got = PolySymmetryCache::symmetryNodeCache.find(key);
        result = got != PolySymmetryCache::symmetryNodeCache.end();

        if (result)
        {
            node = (*got).second.object();
        }
    }

    return result;
}