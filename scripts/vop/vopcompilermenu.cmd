#
# This script is run to generate a list of compilers for VOPs.
# The file is found using the HOUDINI_PATH, so it's possible to
# override the default list with your own.  Or to extend it by calling this
# script first, and then printing your own entries.
#
# There should be two quoted strings for each entry:
#	1)  The actual compiler command
#	2)  The label for the menu entry
#

set common_opts = '-q $VOP_INCLUDEPATH -o $VOP_OBJECTFILE -e $VOP_ERRORFILE $VOP_SOURCEFILE'
set oslc_opts = '-q $VOP_INCLUDEPATH -o $VOP_OBJECTFILE -e $VOP_ERRORFILE $VOP_SOURCEFILE $OSL_INCLUDEPATH'

echo "'vcc $common_opts'" "'VEX Compiler'"
echo "'hrmanshader -cc oslc $oslc_opts'" "'OSL Compiler'"
