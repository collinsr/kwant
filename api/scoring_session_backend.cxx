#include <iostream>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <sstream>

using namespace std;

typedef vector< string > args_t;

typedef function< int( const args_t& ) > scoring_command_t;

int
load_tracks_command( const args_t& args )
{
  cerr << "Load_tracks got " << args.size() << " arguments\n";
  return 1;
}

int
load_detections_command( const args_t& args )
{
  cerr << "Load_detections got " << args.size() << " arguments\n";
  return 2;
}

int main( int argc, char *argv[] )
{
  map< string, scoring_command_t > dispatch_table;
  dispatch_table[ "load_tracks" ] = load_tracks_command;
  dispatch_table[ "load_detections" ] = load_detections_command;

  string line;
  while ( getline( cin, line ))
  {
    istringstream iss( line );
    string tmp;
    args_t args;
    while ( (iss >> tmp ))
    {
      args.push_back( tmp );
    }
    if (args.empty()) continue;

    string key = args[0];
    auto probe = dispatch_table.find( key );
    if (probe == dispatch_table.end())
    {
      cerr << "Unknown keyword '" << key << "'\n";
    }
    else
    {
      int rc = probe->second( args );
      cerr << "'" << key << "' returned " << rc << "\n";
    }
  }
  cout << "All done.\n";
}
