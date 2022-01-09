import os
import re

#Replace region class names, variable names and event names.
#Replace active object class names, variable names and event names.

#Return dictionary holding class, variable and event names of the specified class.
def GetNames(baseName):
    token = re.sub('(?<!^)([A-Z][a-z]+)', r' \1', baseName).split()
    token[0] = token[0].lower()
    retVal = {}
    retVal['class'] = baseName
    retVal['var'] = "".join(token)
    retVal['event'] = "_".join([t.upper() for t in token])
    return retVal

def ReplaceNames(buf, fromNames, toNames):
    for key in fromNames.keys():
        buf = re.sub(fromNames[key], toNames[key], buf)
    return buf

#Process file list.
def ProcessFiles(fileList, tempAct, tempReg, actObj, region):
    if tempAct:
        tempActNames = GetNames(tempAct)
        actObjNames = GetNames(actObj)
        print(tempActNames)
        print(actObjNames)
    if tempReg:
        tempRegNames = GetNames(tempReg)
        regionNames = GetNames(region)
        print(tempRegNames)
        print(regionNames)
    for fileName in fileList:
        if os.path.isfile(fileName):
            print(fileName)
            f = open(fileName, 'rt')
            buf = f.read()
            f.close()
            #Do region before actObj.
            if tempReg:
                buf = ReplaceNames(buf, tempRegNames, regionNames)
            if tempAct:
                buf = ReplaceNames(buf, tempActNames, actObjNames)
            f = open(fileName, 'wt')    
            f.write(buf)    
            f.close()     
