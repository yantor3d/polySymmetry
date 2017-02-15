"""polySymmetry

The _initializePlugin and _uninitializePlugin functions are automatically 
called when the plugin is un/loaded to create the polySymmetry tools menu.

"""

import polySymmetry.commands
import polySymmetry.tools

from maya.api import OpenMaya
from maya import cmds 


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

    def __init__(self, name, action=None):
        self.name = name 
        self.action = action 

    def __str__(self):
        return self.name.replace(' ', '')

    def __call__(self, *args, **kwargs):
        try:
            self.action()
        except Exception as e:
            OpenMaya.MGlobal.displayError(str(e))


_tools = polySymmetry.tools 
_cmds = polySymmetry.commands 


_POLY_SYMMETRY_MENU_ITEMS = (
    _MenuItem('Poly Symmetry Tool', _tools.polySymmetryTool),
    None,
    _MenuItem('Flip Mesh', _cmds.flipMesh),
    _MenuItem('Mirror Mesh', _cmds.mirrorMesh),
    None,
    _MenuItem("Copy Poly Deformer Weights", _cmds.copyPolyDeformerWeights),    
    _MenuItem("Flip Poly Deformer Weights", _cmds.flipDeformerWeights),
    _MenuItem("Mirror Poly Deformer Weights", _cmds.mirrorDeformerWeights),
    None,    
    _MenuItem("Copy Poly Skin Weights", _cmds.copyPolySkinWeights),
    _MenuItem("Mirror Poly Skin Weights", _cmds.mirrorPolySkinWeights),
)


def _try_delete_menu():
    if cmds.menu(_POLY_SYMMETRY_MENU_NAME, query=True, exists=True):
        cmds.deleteUI(_POLY_SYMMETRY_MENU_NAME)   
        

def _initializePlugin():
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
        if item is None:
            cmds.menuItem(divider=True)
            continue 

        cmds.menuItem(
            label=item.name,
            command=item, 
            sourceType='python'
        )
        
    cmds.menuSet("riggingMenuSet", addMenu=_POLY_SYMMETRY_MENU_NAME)
    
    
def _uninitializePlugin():
    """Construct the poly symmetry plugin menu."""

    if cmds.about(batch=True):
        return
    
    _try_delete_menu()