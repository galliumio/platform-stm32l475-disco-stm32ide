import os
import re
import sys
from distutils.dir_util import copy_tree
import AddActHelper as helper

if len(sys.argv) < 2:
    print("Enter active object name.")
    exit()

tempAct = 'SimpleAct'
actObj = sys.argv[1]

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

#Replace class, varialble and event names in new files.        
helper.ProcessFiles(actFiles, tempAct, '', actObj, '')

