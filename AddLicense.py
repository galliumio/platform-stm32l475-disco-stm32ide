import os, fnmatch
def FindFile(basePath, pattern):
    for pathName, dirList, fileList in os.walk(basePath):
        for fileName in fileList:
            if fnmatch.fnmatch(fileName, pattern):
                foundFile = os.path.join(pathName, fileName)
                yield foundFile
                
#from enum import Enum
#class State(Enum):

import re
def ReplaceHeader(sourceFile, licenseFile):
    licenseRead = open(licenseFile, 'rt')
    licenseBuf = licenseRead.read()  
    licenseRead.close()    
    sourceRead = open(sourceFile, 'rt')
    sourceBuf = sourceRead.read()
    sourceRead.close()    
    sourceWrite = open(sourceFile, 'wt')    
    sourceBuf = re.sub('/\*((.|\n)*?)Copyright \(C\) Gallium Studio LLC((.|\n)*?)\*/(\s*)', '', sourceBuf)
    sourceWrite.write(licenseBuf)
    sourceWrite.write(sourceBuf)    
    sourceWrite.close() 
 
subdir = ['src', 'include', 'framework'] 
fileExt = ['*.cpp', '*.c', '*.h']
exclude = ['_write.c', 'stm32l4xx_hal_conf.h']
  
def IsExcluded(fileName):
    for exFile in exclude:
        m = re.search(exFile+'$', fileName)
        if m:
            return True;        
    return False
  
for baseDir in subdir:  
    for extension in fileExt:
        for fileName in FindFile(baseDir, extension):
            if IsExcluded(fileName):
                print('****' + fileName + " excluded ****")
            else:
                print(fileName)
                ReplaceHeader(fileName, 'LICENSE.txt')
