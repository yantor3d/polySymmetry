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
    
**Synopsis**
```
polySymmetry 
    [-constructionHistory], 
    [-exists], 
    [-leftSideVertex], 
    [-symmetry]
    [mesh or polySymmetryData]
```

polySymmetry is undoable, queryable, and **NOT editable**.

Returns a component symmetry table for a polygon mesh.

If constructionHistory is on, the symmetry table is baked into a polySymmetryData node. Otherwise, it is encoded as a JSON string. 

In query mode, return type is based on queried flag(s) and the node argument.

**Long name (short name)** | Argument types | Properties
:-------------------------- | :-------------- | :----------
**constructionHistory (ch)** | *boolean* | [C]
    If true, bake the symmetry table to a node and return the node. Otherwise, encode the symmetry table as a JSON object and return it. Default: true. | |
**exists (ex)** | *boolean* | [Q]
Return true if a polySymmetryData node exists for the selected mesh.  | |
**leftSideVertex (lsv)** | *component* | [C]
Component name (eg, vtx[7]) of a vertex on the *left* side of the mesh. There must be at least one left side vertex. | |
**symmetry (sym)** | *componentList* | [C]
Components (eg, f[13] f[37] e[41] e[7] vtx[3] vtx[501]) that are known to be symmetrical. Must provide exactly two faces, two edges, and two vertices. Each vertex must be on one of the edges, and each edge must be on one of the faces. The edges must have two adjacent faces. | |

**Examples**
```
// This command create the symmetry table. The constructionHistory flag is on by default.
polySymmetry -symmetry f[354] f[603] e[775] e[1103] vtx[350] vtx[503] -leftSideVertex vtx[0] head_MESH;
// Result: polySymmetryData1 // 
```
```
// Turning the construction history flag off will return the symmetry table as a JSON object.
polySymmetry -symmetry f[354] f[603] e[775] e[1103] vtx[350] vtx[503] -leftSideVertex vtx[0] -ch 0 head_MESH;
// Result truncated for brevity.
// Result:: { "e": { "symmetry": [...], "sides": [...] }, "f": { ... }, "vtx": { ... }} //
```
```
// polySymmetryData nodes are cached in memory based on a checksum of the mesh's topology. 
// You can query the cache with a mesh to retrieve the matching polySymmetryData node.
// If there is no match, an empty string is returned.
polySymmetry -q myMesh;
// Result: polySymmetryData1
```
```
// You can query the cache with a mesh to ascertain whether or not there is a matching polySymmetryData node.
polySymmetry -q -ex yourMesh;
// Result: 0
```
```
// You can query a polySymmetryData node to get the symmetry table as a JSON object.
polySymmetry -q polySymmetryData1
// Result { "e": { "symmetry": [...], "sides": [...] }, "f": { ... }, "vtx": { ... }} //
```
