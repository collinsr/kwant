#
#
#

SBB_EXE = '/Users/collinsr/code/pub/kwant-collinsr-bin/bin/scoring_session_backend'
import subprocess
import json

proc = subprocess.Popen( [SBB_EXE], stdout=subprocess.PIPE, stderr=subprocess.PIPE )
payload = ''
while True:
    line = proc.stdout.readline()
    line = line.rstrip()
    if line != '':
        if line != 'EOD':
            payload = payload+line
        else:
            data = json.loads( payload )
            print json.dumps( data )
            payload = ''
    else:
        break


