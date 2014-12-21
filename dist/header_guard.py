#!/usr/bin/python

from subprocess import *
from sys import *
from os import rename, remove, path
from datetime import *
import string
def change_file(filename):
        out=[]
        file_name = path.basename(filename)
        file_name = file_name.replace('.','_')
        path_name = string.split(path.dirname(filename),'/')
        path_name = path_name[1:]
        path_name.append(file_name)
        guard = string.join(path_name,'_')+'_'
        guard = guard.upper()
        file_name = file_name.upper()
        print file_name
        print guard
        
        with open(filename, "r") as fi:
                content=fi.readlines()
                
        for i in content:
                if i.find("#ifndef") >= 0 and i.find(file_name) >= 0:
                        i = "#ifndef {}\n".format(guard)
                if i.find("#define") >= 0 and i.find(file_name) >= 0:
                        i = "#define {}\n".format(guard)
                if i.find(" signals:") == -1:
                        i = i.replace("signals:", " signals:")
                out.append(i)

        out[-1] = '#endif  // '+guard+'\n'
        with open(filename+'_tmp', "w") as fi:
                fi.writelines(out)
        rename(filename+'_tmp', filename)
        

if __name__ == "__main__":
        for files in argv[1:]:
                change_file(files)
