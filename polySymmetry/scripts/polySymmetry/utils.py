import json 

import maya.cmds as cmds


class InvalidSelection(ValueError):
    pass


def loadOptions(commandName):
    result = {}

    varName = "{}OptionVar".format(commandName)

    if cmds.optionVar(exists=varName):
        options = cmds.optionVar(query=varName)

        try:
            result = json.loads(options)
        except (ValueError, TypeError) as e:
            cmds.warning("Error loading options for '{}' - {}".format(commandName, e))

    
    return result 
    

def deleteOptions(commandName):    
    varName = "{}OptionVar".format(commandName)

    if cmds.optionVar(exists=varName):
        cmds.optionVar(remove=varName)


def saveOptions(commandName, options):
    varName = "{}OptionVar".format(commandName)

    try:
        stringVal = json.dumps(options)
    except (ValueError, TypeError) as e:
        cmds.warning("Error saving options for '{}' - {}".format(commandName, e))
    else:
        cmds.optionVar(stringValue=(varName, stringVal))