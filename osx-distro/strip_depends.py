import subprocess
import sys
import os
import sys


def my_popen(cmd):
    proc = subprocess.Popen(cmd, shell=True, stdin= subprocess.PIPE, stdout= subprocess.PIPE, stderr= subprocess.PIPE)
    return proc

def strip_depends(strip_path):
    output = my_popen("otool -L %s" % (strip_path,)).stdout.read()    # Find all the dependencies
    for line in output.split('\n'):
        if '/opt/local' in line:    # Need to strip local dependencies
            lib_path=line.split()[0]
            lib_file_name = os.path.basename(lib_path)
            if lib_file_name != os.path.basename(strip_path):    # Don't strip one-self, to prevent infinite recursion.
                os.system("cp %s ../Frameworks" % (lib_path,))
                print "Copying", lib_path
                os.system("install_name_tool -change %s @executable_path/../Frameworks/%s %s" % (lib_path, lib_file_name, strip_path))
                os.system("install_name_tool -id @executable_path/../Frameworks/%s %s" % (lib_file_name, strip_path))
                strip_depends("../Frameworks/%s" % (lib_file_name,))

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print "Usage: %s <path_to_strip>" % (sys.argv[0],)
        sys.exit(1)
    strip_depends(sys.argv[1])