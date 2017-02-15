"""
polySymmetry 

Support script for polySymmetry plugin.
"""

from maya import cmds 

def polySymmetryTool(*args):
    cmds.setToolTo(cmds.polySymmetryCtx())