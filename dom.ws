#
# iceboot.ws, create a workspace for iceboot
# development...
#

#
# select location of cvs root and login for checkouts (must be first)...
#
# cvsroot must be relative!
#
cvsroot ../../icecube
cvslogin :ext:glacier.lbl.gov:/home/icecube/cvsroot

#
# select platforms of interest (must be second!)
#
platforms epxa10 Linux-i386

#
# import directories from projects...
# 
# project is the project to get files from...
# directories are directories in the project...
# if no directories are given, take them all...
#
# import project [directories...]
import hal
import dom-loader
import iceboot iceboot doc
import stf stf stf-apps docs stf-modules
import dom-fpga dom-fpga
import configboot configboot

#
# one-off links...
#
# project: the project to get files from...
# directory: the directory in the project...
# scope: public or private
# file: the file to link -- if none given use all...
# location: directory where link should sit...
# 
# link project directory scope file scope location
link dom-loader booter public boot.S private booter
link dom-loader booter public boot.x private booter

#
# more will go here when simon is done...
#








