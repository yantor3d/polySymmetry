#### polySymmetry (version 0.0.1)
Maya tool for finding the symmetry of a polygon mesh based on the topology.

#### Description
The Poly Symmetry Tool uses an iterative algorithm to compute the symmetry of a mesh that has the same topology on either side of the center edge loop. The tool requires the user to provide pairs of symmetrical components to start (see below) on pair of symmetrical vertex shells (a vertex shell is usually symmertical to itself). After that, the algorithm walks the topology outward from the user provided components and produces a symmetry table for the edges, faces, and vertices.

The symmetry table can be used for mirroring and flipping data associated with the mesh, such as: skinWeights, deformer weights, blendshape targets, etc. Because topology symmetry is indifferent to the world position of the vertices, you could use the symmetry table to mirror weights on a deformed character *without* having to return to the bind pose.

#### Plugin Contents

##### Tools
------------
**polySymmetryCtx**
To active the tool, run this Python command: 
```
    cmds.loadPlugin('polySymmetry')
    cmds.setToolTo(cmds.polySymmetryCtx())
```

Select the mesh you want find the symmetry of, and press Enter.

Select seven components, and press Enter. The seven components should be:
  * A vertex on the left side of the mesh. This vertex must not touch any other selected component.
  * A vertex, an edge, and a face on each side of the mesh. 
    The selected vertex must be on the selected edge, which must be on the 
    selected face. The selected edges must not be border edges.

After each selection of seven, the vertices of the mesh will be colored 
according to which side of the mesh they are on. If any vertices are 
uncolored. Only the vertices on the same shell(s) as the previous 
selection(s) will be colored.

Continue adding selections until the entire mesh is colored.

With nothing selected, press Enter to complete the tool. A new 
polySymmetryData node will be created with the symmetry data baked into.
    
##### Nodes
------------
**polySymmetryData**
  
  By default, the polySymmetry command creates a data node that holds the component symmetry indices.

  Attribute Name | Description | Attribute Type 
  -------------- | -------------- | --------------
  **vertexSymmetry** | Indices of the array correspond to the vertices of the mesh. Values of the array correspond to the index of the opposite vertex. | int array   
  **edgeSymmetry** | Indices of the array correspond to the edges of the mesh. Values of the array correspond to the index of the opposite edge. | int array   
  **faceSymmetry** | Indices of the array correspond to the face of the mesh. Values of the array correspond to the index of the opposite face. | int array  
  **vertexSides** | Indices of the array correspond to the vertices of the mesh. Values of the array correspond to the which side of the mesh the vertex is on (Left = 1, Center = 0, Right = -1) | int array
  **edgeSides** | Indices of the array correspond to the edges of the mesh. Values of the array correspond to the which side of the mesh the edge is on (Left = 1, Center = 0, Right = -1) | int array 
  **faceSides** | Indices of the array correspond to the faces of the mesh. Values of the array correspond to the which side of the mesh the face is on (Left = 1, Center = 0, Right = -1) | int array
  
##### Commands
------------
**polySymmetry**
    
When completed, the polySymmetry tool prints the equivalent MEL command for querying the symmetry of a mesh. 
You can use this command to return the symmetry as a JSON string if you call it with the -constructionHistory flag set to false. You only need to pass the component names to the `-symmetry` and `-leftSideVertex` flags, not the entire DAG path.

```
polySymmetry -symmetry f[354] f[603] e[775] e[1103] vtx[350] vtx[503] -leftSideVertex vtx[0] head_MESH;
// Result: meshSymmetryData1 // 
```

You can use the -query flag to return the symmetry as a JSON strong from an existing polySymmetryData node. The example above omits some data for brevity. You can use the Python `json` module to decode the JSON data as a dictionary.

```
polySymmetry -q meshSymmetryData1
// Result { "edge": { "symmetry": [...], "sides": [...] }, "faces": { ... }, "vertices": { ... }} //
```
