#
# iceboot.ws, create a workspace for iceboot
# development...
#

#
# select location of cvs root and login for checkouts (must be first)...
#
# cvsroot must be relative!
#
cvsroot ..

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
import iceboot iceboot iceboot-docs
import stf stf stf-apps stf-docs stf-sfe std-tests int-tests fat-tests
import dom-cal dom-cal
import domapp domapp

#
# one-off links...
#
# project: the project to get files from...
# directory: the directory in the project...
# scope: public or private
# file: the file to link -- if * given use all...
# location: directory where link should sit...
# 
# link project directory scope file scope location
link dom-loader booter public boot.S private booter
link dom-loader booter public boot.x private booter

link testdomapp expControl public * public expControl
link testdomapp msgHandler public * public msgHandler
link testdomapp dataAccess public * public dataAccess
link testdomapp domapp_common public * public domapp_common
link testdomapp message public * public message
link testdomapp slowControl public * public slowControl

link testdomapp domapp private * private testdomapp
link testdomapp expControl private * private testdomapp
link testdomapp msgHandler private * private testdomapp
link testdomapp dataAccess private * private testdomapp
link testdomapp domapp_common private * private testdomapp
link testdomapp message private * private testdomapp
link testdomapp slowControl private * private testdomapp

link testdomapp expControl private expControl.h public expControl
link testdomapp dataAccess private dataAccess.h public dataAccess
link testdomapp dataAccess private dataAccessRoutines.h public dataAccess
link testdomapp dataAccess private moniDataAccess.h public dataAccess
link testdomapp dataAccess private compressEvent.h public dataAccess
link testdomapp slowControl private domSControl.h public slowControl
link testdomapp slowControl private domSControlRoutines.h public slowControl

#
# more will go here when simon is done...
#








