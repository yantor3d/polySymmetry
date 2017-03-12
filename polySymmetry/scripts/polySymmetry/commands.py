import contextlib

import maya.api.OpenMaya as OpenMaya
import maya.cmds as cmds
import maya.mel as mel

import polySymmetry.utils 


_INFO = OpenMaya.MGlobal.displayInfo
_ERR = OpenMaya.MGlobal.displayError
_WARN = OpenMaya.MGlobal.displayWarning


def copyPolyDeformerWeights(*args):
    """Copies the weights from one deformer to another.
    
    Parameters
    ----------
    *args : str
        List of one or two meshes, and two deformers. If empty, the active 
        selection is used.
        
    Raises
    ------
    RuntimeError
        If the selected deformers do not deform the selected mesh(es), or if 
        the selected mesh(es) do not have poly symmetry data computed.

    """

    selectedMeshes = _getSelectedMeshes(*args)
    selectedDeformers = _getSelectedDeformers(*args)

    if len(selectedMeshes) not in (1, 2) or len(selectedDeformers) != 2:
        _ERR(
            "Must select a source mesh, a destination mesh (optional), "
            "a source deformer, and a destination deformer."
        )
        return

    kwargs = {
        'sourceMesh': selectedMeshes[0],
        'sourceDeformer': selectedDeformers[0],
        'destinationMesh': selectedMeshes[-1],
        'destinationDeformer': selectedDeformers[1],
        'direction': 1
    }

    cmds.polyDeformerWeights(**kwargs)

    _INFO(
        "Result: Copied weights from {sourceDeformer} ({sourceMesh}) "
        "to {destinationDeformer} ({destinationMesh})".format(**kwargs)
    )


def flipDeformerWeights(*args):
    """Flips the weights on the selected deformer(s).
    
    Parameters
    ----------
    *args : str
        List of equal number of meshes and deformers. If empty, the active 
        selection is used.

    Raises
    ------
    RuntimeError
        If the selected deformers do not deform the selected meshes, or if 
        the selected meshes do not have poly symmetry data computed.
        
    """

    polyDeformerWeights(*args, flip=True)


def mirrorDeformerWeights(*args):
    """Mirrors the weights on the selected deformer(s).
    
    Parameters
    ----------
    *args : str
        List of equal number of meshes and deformers. If empty, the active 
        selection is used.
        
    Raises
    ------
    RuntimeError
        If the selected deformers do not deform the selected meshes, or if 
        the selected meshes do not have poly symmetry data computed.

    """

    polyDeformerWeights(*args, mirror=True)


def polyDeformerWeights(*args, **kwargs):
    """Copies the weights from one deformer to another.
    
    Parameters
    ----------
    *args : str
        List of equal number of meshes and deformers. If empty, the active 
        selection is used.

    Raises
    ------
    RuntimeError
        If the selected deformers do not deform the selected meshes, or if 
        the selected meshes do not have poly symmetry data computed.
        
    """

    selectedMeshes = _getSelectedMeshes(*args)
    selectedDeformers = _getSelectedDeformers(*args)

    if not selectedMeshes or not selectedDeformers:
        raise polySymmetry.utils.InvalidSelection("Must select at least one mesh and one deformer.")
        return

    if len(selectedMeshes) != len(selectedDeformers):
        raise polySymmetry.utils.InvalidSelection("Must select exactly one deformer per mesh.")
        return

    logMsg = "Result: {} weights on {} ({})."
    actionName = 'Flipped' if 'flip' in kwargs else 'Mirrored'

    with undoChunk():
        for mesh, deformer in zip(selectedMeshes, selectedDeformers):
            cmds.polyDeformerWeights(
                sourceMesh=mesh,
                sourceDeformer=deformer,
                destinationMesh=mesh,
                destinationDeformer=deformer,
                **kwargs
            )

            _INFO(logMsg.format(actionName, mesh, deformer))
   

def flipMesh(*args):
    """Flips the vertex positions of the selected mesh(es).
    
    Parameters
    ----------
    *args : str
        List of meshes. If empty, the active selection is used.
        
    Raises
    ------
    RuntimeError
        If the selected mesh(es) do not have poly symmetry data computed.

    """

    selectedMeshes = _getSelectedMeshes(*args)

    if not selectedMeshes:
        raise polySymmetry.utils.InvalidSelection("Select a mesh and try again.")
        return

    with undoChunk():
        for mesh in selectedMeshes:
            cmds.polyFlip(mesh, objectSpace=True)


def mirrorMesh(*args):
    """Mirrors the vertex positions of the selected mesh(es).
    
    Parameters
    ----------
    *args : str
        A base mesh and one or more target meshes. If empty, the active 
        selection is used.
        
    Raises
    ------
    RuntimeError
        If the base mesh and the target mesh(es) are not point compatible, 
        or if the selected mesh(es) do not have poly symmetry data computed.

    """

    selectedMeshes = _getSelectedMeshes(*args)

    if len(selectedMeshes) < 2:
        raise polySymmetry.utils.InvalidSelection("Select a base mesh and a target mesh and try again.")
        return

    baseMesh = selectedMeshes[0]

    with undoChunk():
        for mesh in selectedMeshes[1:]:
            cmds.polyMirror(baseMesh, mesh)


def copyPolySkinWeights(*args):
    """Copies the skinCluster weights from one mesh to another.
    
    Parameters
    ----------
    *args : str
        Two skinned polygon meshes. If empty, the active selection is used.
        
    Raises
    ------
    RuntimeError
        If the source mesh and target mesh are not point compatible.

    """

    try:
        sourceMesh, destinationMesh = _getSelectedMeshes(*args)
    except ValueError:
        raise polySymmetry.utils.InvalidSelection("Must select a souce mesh and a destination mesh.")
        return

    sourceSkin = _getSkinCluster(sourceMesh)
    destinationSkin = _getSkinCluster(destinationMesh)

    if not sourceSkin:
        raise polySymmetry.utils.InvalidSelection("'%s' is not skinned." % sourceMesh)
        return

    if not destinationSkin:
        raise polySymmetry.utils.InvalidSelection("'%s' is not skinned." % destinationMesh)
        return

    cmds.polySkinWeights(
        sourceMesh=sourceMesh,
        sourceSkin=sourceSkin,
        destinationMesh=destinationMesh,
        destinationSkin=destinationSkin,
        direction=1,
        normalize=True
    )


def mirrorPolySkinWeights(*args, **kwargs):
    """Mirrors the skinCluster weights on the selected mesh(es).
    
    Parameters
    ----------
    *args : str
        List of skinned polygon meshes. If empty, the active selection is used.
        
    Raises
    ------
    RuntimeError
        If the selected mesh(es) do not have poly symmetry data computed.

    """

    activeSelection = OpenMaya.MGlobal.getActiveSelectionList()
    numberOfSelectedItems = activeSelection.length()

    selectedMeshes = _getSelectedMeshes(*args)
    skinClusters = [_getSkinCluster(mesh) for mesh in selectedMeshes]

    if not selectedMeshes:
        raise polySymmetry.utils.InvalidSelection("Select a skinned mesh and try again.")

    if not kwargs:
        options = polySymmetry.utils.loadOptions('polySkinWeights')

        kwargs = {'mirror': True}

        if options.get('normalize', False):
            kwargs['normalize'] = True 

        if options.get('useInfluencePattern', True):
            kwargs['influenceSymmetry'] = (
                options.get('leftPattern', 'L_*'),
                options.get('rightPattern', 'R_*')
            )

        kwargs['direction'] = 1 if options.get('direction', 1) == 1 else -1

    with undoChunk():
        for mesh, skin in zip(selectedMeshes, skinClusters):

            if not skin:
                _WARN("Skipping '%s' since it is not skinned." % mesh)
                continue

            cmds.polySkinWeights(
                sourceMesh=mesh,
                sourceSkin=skin,
                destinationMesh=mesh,
                destinationSkin=skin,
                **kwargs
            )


def setInfluenceSymmetry(*args, **kwargs):
    """Sets the symmetry attributes of the influences for the selected mesh(es).
    
    Parameters
    ----------
    *args : str
        List of skinned polygon meshes. If empty, the active selection is used.
    **leftPattern : str
        Pattern for matching influences on the left side of the mesh. 
    **rightPattern : str 
        Pattern for matching influences on the right side of the mesh.         
    """

    selectedMeshes = _getSelectedMeshes(*args)

    leftPattern = kwargs.get('leftPattern', 'L_*')
    rightPattern = kwargs.get('leftPattern', 'R_*')

    if not selectedMeshes:
        raise polySymmetry.utils.InvalidSelection("Select a skinned mesh and try again.")

    with undoChunk():
        for mesh in selectedMeshes:
            skin = _getSkinCluster(mesh)

            if not skin:
                _WARN("Skipping '%s' since it is not skinned." % mesh)
                continue

            cmds.polySkinWeights(
                skin,
                edit=True,
                influenceSymmetry=(leftPattern, rightPattern)
            )


def printInfluenceSymmetry(*args):
    """Sets the symmetry attributes of the influences for the selected mesh(es).
    
    Parameters
    ----------
    *args : str
        List of skinned polygon meshes. If empty, the active selection is used.
        
    """

    selectedMeshes = _getSelectedMeshes(*args)

    if not selectedMeshes:
        raise polySymmetry.utils.InvalidSelection("Select a skinned mesh and try again.")

    with undoChunk():
        for mesh in selectedMeshes:
            skin = _getSkinCluster(mesh)

            if not skin:
                _WARN("Skipping '%s' since it is not skinned." % mesh)
                continue

            result = cmds.polySkinWeights(
                skin,
                query=True,
                influenceSymmetry=True
            )

            print('Mesh: {}'.format(mesh))
            print('Skin: {}'.format(skin))
            print('\n'.join(['{:>32} : {:>32}'.format(*sym.split(':')) for sym in result]))


def _getSkinCluster(mesh):
    """Returns the skinCluster that deforms `mesh`.
    
    Parameters
    ----------
    mesh : str
        Name of a polygon mesh.

    Returns
    -------
    str
        Name of a skinCluster.
        
    """

    return mel.eval('findRelatedSkinCluster "%s"' % mesh)


def _getSelection(*args):
    """Returns the user specified selection, or the scene selection.
    
    Parameters
    ----------
    *args : str
        User specified selection. If empty, the active selection is used.

    Returns
    -------
    list
        List of selected nodes.
        
    """

    if args:
        selection = OpenMaya.MSelectionList()

        for arg in args:
            try:
                selection.add(arg)
            except RuntimeError:
                raise NameError("No object matches name '%s'" % arg)
    else:
        selection = OpenMaya.MGlobal.getActiveSelectionList()

    return selection


def _getSelectedNodes(fnType, *args):
    """Returns the selected nodes of type `fnType`.
    
    Parameters
    ----------
    fnType : OpenMaya.MFn
        Node type enum. Only nodes of this type will be returned.
    *args : str
        User specified selection. If empty, the active selection is used.

    Returns
    -------
    list
        List of selected nodes.
        
    """

    selection = _getSelection(*args)
    numberOfSelectedItems = selection.length()

    result = []

    for i in range(numberOfSelectedItems):
        obj = selection.getDependNode(i)

        if obj.hasFn(fnType):
            result.append(OpenMaya.MFnDependencyNode(obj).name())

    return result    


def _getSelectedDeformers(*args):
    """Returns the selected deformers.
    
    Parameters
    ----------
    *args : str
        User specified selection. If empty, the active selection is used.

    Returns
    -------
    list
        List of selected deformers.

    """

    return _getSelectedNodes(OpenMaya.MFn.kWeightGeometryFilt, *args)


def _getSelectedMeshes(*args):
    """Returns the selected meshes.
    
    Parameters
    ----------
    *args : str
        User specified selection. If empty, the active selection is used.

    Returns
    -------
    list
        List of selected meshes.
        
    """

    selection = _getSelection(*args)
    numberOfSelectedItems = selection.length()

    result = []

    for i in range(numberOfSelectedItems):
        try:
            dagPath = selection.getDagPath(i)
        except TypeError:
            continue

        if dagPath.hasFn(OpenMaya.MFn.kMesh):
            result.append(dagPath.partialPathName())

    return result     


@contextlib.contextmanager
def undoChunk():
    """Wraps a block of code in an undo chunk."""

    try:
        cmds.undoInfo(openChunk=True)
        yield
    except:
        raise
    finally:
        cmds.undoInfo(closeChunk=True)
