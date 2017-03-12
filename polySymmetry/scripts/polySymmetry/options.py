import json 
import webbrowser

from maya import OpenMaya
from maya import cmds
from maya import mel

import polySymmetry.commands as _cmds
import polySymmetry.utils

_INFO = OpenMaya.MGlobal.displayInfo
_ERR = OpenMaya.MGlobal.displayError
_WARN = OpenMaya.MGlobal.displayWarning

_HELP_URL = r"https://github.com/yantor3d/{}/wiki"


class ButtonCommand(object):
    """Options box base class for polySymmetry plugin command."""
    
    def __init__(self, name, callback):
        self.name = name 
        self.callback = callback 

    def __str__(self):
        return self.name 

    def __call__(self, *args, **kwargs):
        try:
            self.callback()
        except Exception as e:
            OpenMaya.MGlobal.displayError(str(e).strip())
        

class _OptionBox(object):
    """Options box base class for polySymmetry plugin command."""

    NICE_NAME = "Poly Symmetry"
    COMMAND_NAME = "polySymmetry"

    def __init__(self, *args, **kwargs):
        windowName = "{}OptionBox".format(self.COMMAND_NAME)
        
        if cmds.window(windowName, exists=True):
            cmds.deleteUI(windowName)
            
        self.win = cmds.window(
            windowName,
            title='{} Options'.format(self.NICE_NAME),
            menuBar=True,
            widthHeight=(546, 350)
        )

        self.editMenu = cmds.menu(label='Edit', parent=self.win)

        cmds.menuItem(label='Save Settings', command=self._handleSaveSettingsClick)
        cmds.menuItem(label='Reset Settings', command=self._handleResetSettingsClick)

        self.helpMenu = cmds.menu(label='Help', parent=self.win)

        cmds.menuItem(label='Help on {}'.format(self.NICE_NAME), command=self._handleHelpClick)

        formLayout = cmds.formLayout()

        self.mainLayout = cmds.formLayout(parent=formLayout)

        buttonLayout = cmds.formLayout(parent=formLayout, numberOfDivisions=100)

        self.applyAndCloseBtn = cmds.button(
            recomputeSize=0, 
            height=26, 
            label='Apply And Close',
            command=ButtonCommand(self.COMMAND_NAME, self._handleApplyAndCloseClick)
            )

        self.applyBtn = cmds.button(
            recomputeSize=0, 
            height=26, 
            label='Apply',
            command=ButtonCommand(self.COMMAND_NAME, self._handleApplyClick)
            )

        self.closeBtn = cmds.button(
            recomputeSize=0, 
            height=26, 
            label='Close',
            command=self._handleCloseClick
            )


        cmds.formLayout(
            buttonLayout,
            edit=True,
            attachForm=(
                (self.applyAndCloseBtn, "top", 0),
                (self.applyAndCloseBtn, "left", 0),
                (self.applyAndCloseBtn, "bottom", 0),

                (self.applyBtn, "top", 0),
                (self.applyBtn, "bottom", 0),

                (self.closeBtn, "top", 0),
                (self.closeBtn, "right", 0),
                (self.closeBtn, "bottom", 0),
            ),
            attachPosition=(
                (self.applyAndCloseBtn, "right", 2, 33),
                (self.applyBtn, "left", 2, 33),
                (self.applyBtn, "right", 2, 67),
                (self.closeBtn, "left", 2, 67),
            )
        )

        cmds.formLayout(
            formLayout,
            edit=True,
            attachForm=(
                (self.mainLayout, "top", 0),
                (self.mainLayout, "left", 0),
                (self.mainLayout, "right", 0),

                (buttonLayout, "left", 4),
                (buttonLayout, "bottom", 4),
                (buttonLayout, "right", 4),
            ),
            attachNone=(
                (buttonLayout, "top")
            ),
            attachControl=(
                (self.mainLayout, "bottom", 4, buttonLayout)
            )
        )

        self.initUI()
        self.show()

    def initUI(self):
        mainLayout= cmds.tabLayout(tabsVisible=0, parent=self.mainLayout)
        cmds.tabLayout(tabsVisible=0)

        cmds.formLayout(
            self.mainLayout,
            edit=True,
            attachForm=(
                (mainLayout, 'top', 0),
                (mainLayout, 'left', 0),
                (mainLayout, 'bottom', 0),
                (mainLayout, 'right', 0)
            )
        )

    def show(self):
        if cmds.window(self.win, exists=True):
            cmds.showWindow(self.win)
            self.loadOptions()

    def close(self, *args):
        if cmds.window(self.win, exists=True):
            cmds.evalDeferred('cmds.deleteUI("{}")'.format(self.win))

    def _handleApplyClick(self, *args):
        try:
            self.doIt()
        except _cmds.InvalidSelection as e:
            _ERR(e)
        else:
            self.saveOptions()

    def _handleApplyAndCloseClick(self, *args):        
        try:
            self.doIt()
        except _cmds.InvalidSelection as e:
            _ERR(e)
        else:
            self.saveOptions()
            self.close()
    
    def _handleCloseClick(self, *args):
        self.close()

    def _handleSaveSettingsClick(self, *args):
        self.saveOptions()

    def _handleResetSettingsClick(self, *args):
        self.resetOptions()

    def _handleHelpClick(self, *args):
        self.help()
    
    def help(self):
        helpUrl = _HELP_URL.format(self.COMMAND_NAME)
        webbrowser.open(helpUrl)

    def doIt(self):
        raise NotImplementedError()

    def saveOptions(self):
        options = self.getOptions()
        polySymmetry.utils.saveOptions(self.COMMAND_NAME, options)

    def loadOptions(self):
        options = polySymmetry.utils.loadOptions(self.COMMAND_NAME)        
        self.setOptions(options)

    def resetOptions(self):
        polySymmetry.utils.deleteOptions(self.COMMAND_NAME)
        self.setOptions({})

    def setOptions(self, options):
        pass

    def getOptions(self):
        return {}


class PolyDeformerWeightsOptionBox(_OptionBox):
    """Options box for polyDeformerWeights command."""

    NICE_NAME = "Poly Deformer Weights"
    COMMAND_NAME = "polyDeformerWeights"

    def __init__(self, *args, **kwargs):
        super(self.__class__, self).__init__(*args, **kwargs)

        if 'default_action' in kwargs:
            _setChoice(self.actionWidget, kwargs['default_action'])
            self._refreshUI()

    def initUI(self):
        super(self.__class__, self).initUI()

        cmds.columnLayout(adjustableColumn=True, rowSpacing=3)

        self.actionWidget = cmds.radioButtonGrp(
            label='Action',            
            numberOfRadioButtons=3,
            label1='Copy',
            label2='Flip',
            label3='Mirror',
            select=3,
            changeCommand=self._refreshUI
        )

        self.directionWidget = cmds.radioButtonGrp(
            label="Direction",            
            numberOfRadioButtons=2,
            label1="Left to Right",
            label2="Right To Left",
            select=1
        )

    def _refreshUI(self, *args):
        action = _getChoice(self.actionWidget)

        cmds.radioButtonGrp(
            self.directionWidget,
            edit=True,
            enable=action == 3
        )

    def doIt(self):
        options = self.getOptions()

        kwargs = {}

        if options['action'] == 2:
            kwargs['flip'] = True
        elif options['action'] == 3:
            kwargs['mirror'] = True

        kwargs['direction'] = 1 if options['direction'] == 1 else -1

        if option['action'] == 1:
            _cmds.copyPolyDeformerWeights()
        else:
            _cmds.polyDeformerWeights(**kwargs)
                
    def getOptions(self):
        return {
            "direction": _getChoice(self.directionWidget),
            "action": _getChoice(self.actionWidget)
        }

    def setOptions(self, options):
        _setChoice(self.directionWidget, options.get('direction', 1))
        _setChoice(self.actionWidget, options.get('action', 3))

        self._refreshUI()


class InfluenceSymmetryOptionsBox(_OptionBox):
    """Options box for polySkinWeights -influenceSymmetry command."""

    NICE_NAME = "Influence Symmetry"
    COMMAND_NAME = "influenceSymmetry"

    def initUI(self):
        super(self.__class__, self).initUI()

        cmds.columnLayout(adjustableColumn=True, rowSpacing=3)

        self.leftPatternWidget = cmds.textFieldGrp(
            label='Left Pattern',
            text='L_*'
        )

        self.rightPatternWidget = cmds.textFieldGrp(
            label='Right Pattern',
            text='R_*'
        )

    def doIt(self):
        options = self.getOptions()

        _cmds.setInfluenceSymmetry(**options)
                
    def getOptions(self):
        return {
            "leftPattern": _getText(self.leftPatternWidget),
            "rightPattern": _getText(self.rightPatternWidget)
        }

    def setOptions(self, options):    
        _setText(self.leftPatternWidget, options.get('leftPattern', 'L_*'))
        _setText(self.rightPatternWidget, options.get('rightPattern', 'R_*'))


class PolySkinWeightsOptionBox(_OptionBox):
    """Options box for polySkinWeights command."""

    NICE_NAME = "Poly Skin Weights"
    COMMAND_NAME = "polySkinWeights"

    def __init__(self, *args, **kwargs):
        super(self.__class__, self).__init__(*args, **kwargs)

        if 'default_action' in kwargs:
            _setChoice(self.actionWidget, kwargs['default_action'])
            self._refreshUI()

    def initUI(self):
        super(self.__class__, self).initUI()

        cmds.columnLayout(adjustableColumn=True, rowSpacing=3)

        self.actionWidget = cmds.radioButtonGrp(
            label='Action',            
            numberOfRadioButtons=3,
            label1='Copy',
            label2='Flip',
            label3='Mirror',
            select=3,
            changeCommand=self._refreshUI
        )

        self.directionWidget = cmds.radioButtonGrp(
            label="Direction",            
            numberOfRadioButtons=2,
            label1="Left to Right",
            label2="Right To Left",
            select=1
        )

        cmds.separator()

        self.useInfluencePatternWidget = cmds.checkBoxGrp(
            label='Use Influence Pattern',
            changeCommand=self._refreshUI,
            numberOfCheckBoxes=1,
            value1=1
        )

        self.leftPatternWidget = cmds.textFieldGrp(
            label='Left Pattern',
            text='L_*'
        )

        self.rightPatternWidget = cmds.textFieldGrp(
            label='Right Pattern',
            text='R_*'
        )

        cmds.separator()

        self.normalizeWidget = cmds.checkBoxGrp(
            label='Normalize',
            numberOfCheckBoxes=1
        )

    def _refreshUI(self, *args):
        action = _getChoice(self.actionWidget)

        cmds.radioButtonGrp(
            self.directionWidget,
            edit=True,
            enable=action == 3
        )

        useInfluencePattern = _getChecked(self.useInfluencePatternWidget)

        cmds.textFieldGrp(
            self.leftPatternWidget,
            edit=True, 
            enable=useInfluencePattern
        )

        cmds.textFieldGrp(
            self.rightPatternWidget,
            edit=True, 
            enable=useInfluencePattern
        )

    def doIt(self):
        options = self.getOptions()

        kwargs = {}

        if options['normalize']:
            kwargs['normalize'] = True 

        if options['action'] == 2:
            kwargs['flip'] = True
        elif options['action'] == 3:
            kwargs['mirror'] = True

        if options['useInfluencePattern']:
            kwargs['influenceSymmetry'] = (
                options['leftPattern'],
                options['rightPattern']
            )

        kwargs['direction'] = 1 if options['direction'] == 1 else -1

        _cmds.mirrorPolySkinWeights(**kwargs)
                
    def getOptions(self):
        return {
            "normalize":  _getChecked(self.normalizeWidget),
            "useInfluencePattern": _getChecked(self.useInfluencePatternWidget),
            "leftPattern": _getText(self.leftPatternWidget),
            "rightPattern": _getText(self.rightPatternWidget),
            "direction": _getChoice(self.directionWidget),
            "action": _getChoice(self.actionWidget)
        }

    def setOptions(self, options):    
        _setText(self.leftPatternWidget, options.get('leftPattern', 'L_*'))
        _setText(self.rightPatternWidget, options.get('rightPattern', 'R_*'))

        _setChecked(
            self.useInfluencePatternWidget, 
            options.get('useInfluencePattern', True)
        )

        _setChecked(self.normalizeWidget, options.get('normalize', True))

        _setChoice(self.directionWidget, options.get('direction', 1))
        _setChoice(self.actionWidget, options.get('action', 3))

        self._refreshUI()


def copyPolyDeformerWeightsOptions():
    PolyDeformerWeightsOptionBox(default_action=1)


def flipPolyDeformerWeightsOptions():
    PolyDeformerWeightsOptionBox(default_action=2)


def mirrorPolyDeformerWeightsOptions():
    PolyDeformerWeightsOptionBox(default_action=3)


def copyPolySkinWeightsOptions():
    PolySkinWeightsOptionBox(default_action=1)


def mirrorPolySkinWeightsOptions():
    PolySkinWeightsOptionBox(default_action=3)


def _getText(widget):
    return cmds.textFieldGrp(widget, query=True, text=True)


def _setText(widget, text):
    cmds.textFieldGrp(widget, edit=True, text=text)


def _getChecked(widget):
    return cmds.checkBoxGrp(widget, query=True, value1=True)


def _setChecked(widget, state):
    cmds.checkBoxGrp(widget, edit=True, value1=state)


def _getChoice(widget):
    return cmds.radioButtonGrp(widget, query=True, select=True)


def _setChoice(widget, choice):
    cmds.radioButtonGrp(widget, edit=True, select=int(choice))