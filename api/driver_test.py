#
# Trying to use the driver object
#

from ssb_driver import SSBDriver
import json

ssb = SSBDriver.SSBDriver()

x = ssb.load_tracks("/Users/collinsr/tmp/1x10.kw18")
y = ssb.load_tracks("/Users/collinsr/tmp/1x10.kw18")

print json.dumps( x )
print json.dumps( y )
