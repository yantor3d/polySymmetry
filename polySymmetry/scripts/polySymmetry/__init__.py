"""polySymmetry

The _initializePlugin and _uninitializePlugin functions are automatically 
called when the plugin is un/loaded to create the polySymmetry tools menu.

"""

import polySymmetry.commands as _cmds
import polySymmetry.options as _opts
import polySymmetry.tools as _tools

import maya.api.OpenMaya as OpenMaya
import maya.cmds as cmds


_POLY_SYMMETRY_MENU_NAME = 'polySymmetryMenu'   


class _MenuItem(object):
    """Simple wrapper to keep the script editor is pretty.
    
    Attributes
    ----------
    name : str
        Label for this menu item.
    action : callable
        Command executed when this menu item is clicked.
    isOptionBox : bool
        If True, this menu item is an option box.

    """

    def __init__(self, name, action=None, isOptionBox=False):
        self.name = name 
        self.action = action 
        self.isOptionBox = isOptionBox

    def __str__(self):
        return self.name.replace(' ', '')

    def __call__(self, *args, **kwargs):
        try:
            self.action()
        except Exception as e:
            OpenMaya.MGlobal.displayError(str(e).strip())


_POLY_SYMMETRY_MENU_ITEMS = (
    _MenuItem('Poly Symmetry Tool', _tools.polySymmetryTool),
    None,
    _MenuItem('Flip Mesh', _cmds.flipMesh),
    _MenuItem('Mirror Mesh', _cmds.mirrorMesh),
    None,
    _MenuItem("Copy Poly Deformer Weights", _cmds.copyPolyDeformerWeights),    
    _MenuItem("Copy Poly Deformer Weights Options", _opts.copyPolyDeformerWeightsOptions, True),    
    _MenuItem("Flip Poly Deformer Weights", _cmds.flipDeformerWeights),
    _MenuItem("Flip Poly Deformer Weights Options", _opts.flipPolyDeformerWeightsOptions, True),    
    _MenuItem("Mirror Poly Deformer Weights", _cmds.mirrorDeformerWeights),
    _MenuItem("Mirror Poly Deformer Weights Options", _opts.mirrorPolyDeformerWeightsOptions, True),    
    None,    
    _MenuItem("Copy Poly Skin Weights", _cmds.copyPolySkinWeights),
    _MenuItem("Copy Poly Skin Weights Options", _opts.copyPolySkinWeightsOptions, True),
    _MenuItem("Mirror Poly Skin Weights", _cmds.mirrorPolySkinWeights),
    _MenuItem("Mirror Poly Skin Weights Options", _opts.mirrorPolySkinWeightsOptions, True),
    None,
    _MenuItem("Set Influence Symmetry", _cmds.setInfluenceSymmetry),
    _MenuItem("Set Influence Symmetry Options", _opts.InfluenceSymmetryOptionsBox, True),
    _MenuItem("Print Influence Symmetry", _cmds.printInfluenceSymmetry),
    None
)


def _try_delete_menu():
    if cmds.menu(_POLY_SYMMETRY_MENU_NAME, query=True, exists=True):
        cmds.deleteUI(_POLY_SYMMETRY_MENU_NAME)   
        

def _initializePlugin(*args):
    """Construct the poly symmetry plugin menu."""
            
    if cmds.about(batch=True):
        return
        
    _try_delete_menu()
    
    cmds.menu(
        _POLY_SYMMETRY_MENU_NAME,
        label="Poly Symmetry", 
        parent='MayaWindow'
    )

    for item in _POLY_SYMMETRY_MENU_ITEMS:
        _addMenuItem(item)

    _addMenuItem(
        _MenuItem('Reload Menu', _initializePlugin)
    )
        
    cmds.menuSet("riggingMenuSet", addMenu=_POLY_SYMMETRY_MENU_NAME)
    

def _addMenuItem(item):
    if item is None:
        cmds.menuItem(divider=True)
    else:
        cmds.menuItem(
            label=item.name,
            command=item, 
            sourceType='python',
            optionBox=item.isOptionBox
        )


def _uninitializePlugin():
    """Construct the poly symmetry plugin menu."""

    if cmds.about(batch=True):
        return
    
    _try_delete_menu()