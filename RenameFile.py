import os
import re
import sys
from distutils.dir_util import copy_tree
import AddActHelper as helper

if len(sys.argv) < 4:
    print("Enter (1) file to change (2) old HSM name (3) new HSM name")
    exit()

hsmFile = sys.argv[1]
oldHsm = sys.argv[2]
newHsm = sys.argv[3]

if not os.path.exists(hsmFile):
    print("File", hsmFile, "does not exist.")
    exit()

#Rename HSM filename
newFile = re.sub(oldHsm + r'([^/]*)$', newHsm + r'\1', hsmFile)
os.rename(hsmFile, newFile)    

#Replace class, varialble and event names in new file.        
print("Processing " + newFile)
helper.ProcessFiles([newFile], '', oldHsm, '', newHsm)
