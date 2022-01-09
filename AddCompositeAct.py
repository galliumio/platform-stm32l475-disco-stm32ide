import os
import re
import sys
from distutils.dir_util import copy_tree
import AddActHelper as helper

if len(sys.argv) < 3:
    print("Enter active object and region name.")
    exit()

tempAct = 'CompositeAct'
tempReg = 'CompositeReg'
actObj = sys.argv[1]
region = sys.argv[2]

src = "./src/Template/" + tempAct
actPath = "./src/" + actObj

if os.path.exists(actPath):
    print("Path", actPath, "already exists.")
    exit()

#Copy from template to new active object folder.
print('Creating {0}...'.format(actPath))
copy_tree(src, actPath)

#Rename active object filenames
actFiles = [actPath + '/' + name for name in os.listdir(actPath)]
for fileName in actFiles:
    if os.path.isfile(fileName):
        os.rename(fileName, re.sub(tempAct + r'([^/]*)$', actObj + r'\1', fileName))
actFiles = [actPath + '/' + name for name in os.listdir(actPath)]        

#Rename region folder.
regPath = actPath + '/' + region
os.rename(actPath + '/' + tempReg, regPath)

#Rename region files.
regFiles = [regPath + '/' + name for name in os.listdir(regPath)]
for fileName in regFiles:
    if os.path.isfile(fileName):
        os.rename(fileName, re.sub(tempReg + r'([^/]*)$', region + r'\1', fileName))
regFiles = [regPath + '/' + name for name in os.listdir(regPath)]        
        
#Replace class, varialble and event names in new files.        
helper.ProcessFiles(actFiles, tempAct, tempReg, actObj, region)
helper.ProcessFiles(regFiles, tempAct, tempReg, actObj, region)

