import os
import re
import sys
from distutils.dir_util import copy_tree
import AddActHelper as helper

if len(sys.argv) < 3:
    print("Enter old HSM name and new HSM name. The HSM folder must be in the current directory.")
    exit()

oldHsm = sys.argv[1]
newHsm = sys.argv[2]

if not os.path.exists(oldHsm):
    print("Old HSM path", oldHsm, "does not exist.")
    exit()
    
if os.path.exists(newHsm):
    print("New HSM path", newHsm, "already exists.")
    exit()    

#Copy from old HSM to new HSM folder.
print('Creating {0}...'.format(newHsm))
copy_tree(oldHsm, newHsm)

#Rename HSM filenames
hsmFiles = [newHsm + '/' + name for name in os.listdir(newHsm)]
for fileName in hsmFiles:
   if os.path.isfile(fileName):
       os.rename(fileName, re.sub(oldHsm + r'([^/]*)$', newHsm + r'\1', fileName))
hsmFiles = [newHsm + '/' + name for name in os.listdir(newHsm)]        

#Replace class, varialble and event names in new files.        
helper.ProcessFiles(hsmFiles, '', oldHsm, '', newHsm)
print('*************************************************')
print('Please manually delete old HSM path {0}'.format(oldHsm))
print('*************************************************')
