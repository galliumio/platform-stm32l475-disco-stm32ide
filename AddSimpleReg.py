import os
import re
import sys
from distutils.dir_util import copy_tree
import AddActHelper as helper

if len(sys.argv) < 3:
    print("Enter base active object name and new region object name")
    exit()

tempReg = 'SimpleReg'
actObj = sys.argv[1]
region = sys.argv[2]

src = "./src/Template/" + tempReg
actPath = "./src/" + actObj
regPath = actPath + '/' + region

if not os.path.exists(actPath):
    print("Base active object path", actPath, "does not exist.")
    exit()
    
if os.path.exists(regPath):
    print("Region path", regPath, "already exists.")
    exit()    

#Copy from template to new region folder.
print('Creating {0}...'.format(regPath))
copy_tree(src, regPath)

#Rename region filenames
regFiles = [regPath + '/' + name for name in os.listdir(regPath)]
for fileName in regFiles:
   if os.path.isfile(fileName):
       os.rename(fileName, re.sub(tempReg + r'([^/]*)$', region + r'\1', fileName))
regFiles = [regPath + '/' + name for name in os.listdir(regPath)]        

#Replace class, varialble and event names in new files.        
helper.ProcessFiles(regFiles, '', tempReg, '', region)

