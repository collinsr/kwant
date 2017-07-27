#
#
#

SBB_EXE = '/Users/collinsr/code/pub/kwant-collinsr-bin/bin/scoring_session_backend'
import subprocess
import json
import sys

class SSBDriver:

    def __init__( self ):
        self.proc = subprocess.Popen( [SBB_EXE], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE )
        r = self.do()
        sys.stderr.write( 'Connection status: %s\n' % json.dumps(r) )

    def do( self ):
        ret = ''
        while True:
            line = self.proc.stdout.readline()
            line = line.rstrip()
            if line != 'EOD':
                ret = ret+line
            else:
                data = json.loads( ret )
                return data

    def load_tracks( self, fn ):
        self.proc.stdin.write( "load_tracks %s\n" % fn )
        return self.do()

