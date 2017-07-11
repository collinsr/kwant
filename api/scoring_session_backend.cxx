#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <sstream>
#include <json.h>

#include <track_oracle/core/track_oracle_api_types.h>
#include <track_oracle/core/track_oracle_core.h>
#include <track_oracle/file_formats/file_format_manager.h>

using namespace std;

using kwiver::track_oracle::track_oracle_core;
using kwiver::track_oracle::file_format_manager;

using kwiver::track_oracle::track_handle_list_type;
using kwiver::track_oracle::handle_list_type;
using kwiver::track_oracle::domain_handle_type;


struct ssb_response_t
{
  string state;  // "complete", "start", "update"
  string status; // "ok", "err"
  string cmd;    // whatever the command was
  ssb_response_t(): state("invalid"), status("invalid"), cmd("invalid") {}

  void complete_error( const string& msg );
  void complete_okay( const string& msg );
  void add_kv( const string& key, const string& val );
  void add_kv( const string& key, const domain_handle_type& val );
  void emit_as_json( ostream& os ) const;
private:
  map <string, string> other_kv;
};

typedef vector< string > args_t;
typedef function< void( const args_t&, ssb_response_t& resp ) > scoring_command_t;

void
ssb_response_t
::complete_error( const string& msg )
{
  this->state = "complete";
  this->status = "err";
  this->add_kv( "msg", msg );
}

void
ssb_response_t
::complete_okay( const string& msg )
{
  this->state = "complete";
  this->status = "ok";
  this->add_kv( "msg", msg );
}

void
ssb_response_t
::add_kv( const string& key, const string& v )
{
  this->other_kv[ key ] = v;
}

void
ssb_response_t
::add_kv( const string& key, const domain_handle_type& v )
{
  ostringstream oss;
  oss << v;
  this->add_kv( key, oss.str() );
}

void
ssb_response_t
::emit_as_json( ostream& os ) const
{
  JSONNode root(JSON_NODE);
  root.set_name("ssb-response");
  root.push_back( JSONNode( "state", this->state ));
  root.push_back( JSONNode( "status", this->status ));
  root.push_back( JSONNode( "cmd", this->cmd ));
  for (auto kv: this->other_kv)
  {
    root.push_back( JSONNode( kv.first, kv.second ));
  }
  os << root.write_formatted() << endl;
  os << "EOD" << endl;
}

void
load_tracks_command( const args_t& args, ssb_response_t& resp )
{
  if (args.size() != 2)
  {
    resp.complete_error( "Incorrect number of arguments" );
    return;
  }
  track_handle_list_type tracks;
  if ( ! file_format_manager::read( args[1], tracks ))
  {
    resp.complete_error( "Failed to load tracks" );
    return;
  }
  handle_list_type h = track_oracle_core::track_to_generic_handle_list( tracks );
  domain_handle_type d = track_oracle_core::create_domain( h );
  resp.add_kv( "tracks", d );
  ostringstream oss;
  oss << "Loaded " << tracks.size() << " tracks";
  resp.complete_okay( oss.str() );
}

void
write_tracks_command( const args_t& args, ssb_response_t& resp )
{
  if (args.size() != 3)
  {
    resp.complete_error( "Incorrect number of arguments" );
    return;
  }
  istringstream iss( args[2] );
  domain_handle_type d;
  if ( ! ( iss >> d ))
  {
    resp.complete_error( "Couldn't extract domain handle from '"+args[2]+"'" );
    return;
  }
  if ( ! track_oracle_core::is_domain_defined( d ))
  {
    resp.complete_error( "No such domain '" + args[2] + "'" );
    return;
  }
  handle_list_type h = track_oracle_core::get_domain( d );
  track_handle_list_type tracks = track_oracle_core::generic_to_track_handle_list( h );
  ofstream os( args[1].c_str() );
  if ( ! os )
  {
    resp.complete_error( "Couldn't open '"+args[1]+"' for writing");
    return;
  }
  if ( ! track_oracle_core::write_kwiver( os, tracks ))
  {
    resp.complete_error( "Couldn't write '"+args[1]+"'" );
    return;
  }
  ostringstream oss;
  oss << "Wrote " << tracks.size() << " tracks";
  resp.complete_okay( oss.str() );
}

void
load_detections_command( const args_t& args, ssb_response_t& resp )
{
}

int main( int argc, char *argv[] )
{
  map< string, scoring_command_t > dispatch_table;
  dispatch_table[ "load_tracks" ] = load_tracks_command;
  dispatch_table[ "load_detections" ] = load_detections_command;
  dispatch_table[ "write_tracks" ] = write_tracks_command;

  {
    ssb_response_t ready;
    ready.state = "complete";
    ready.status = "ok";
    ready.cmd = "ready";
    ready.emit_as_json( cout );
  }

  string line;

  while ( getline( cin, line ))
  {
    istringstream iss( line );

    ssb_response_t resp;
    resp.cmd = line;
    string tmp;

    args_t args;
    while ( (iss >> tmp ))
    {
      args.push_back( tmp );
    }
    if (args.empty())
    {
      resp.state = "complete";
      resp.status = "ok";
    }
    else
    {
      string key = args[0];
      auto probe = dispatch_table.find( key );
      if (probe == dispatch_table.end())
      {
        resp.state = "complete";
        resp.status = "err";
        resp.complete_error( "unknown keyword '"+key+"'" );
      }
      else
      {
        probe->second( args, resp );
      }
    }
    resp.emit_as_json( cout );
  }
  ssb_response_t done;
  done.state = "complete";
  done.status = "ok";
  done.cmd = "quit";
  done.emit_as_json( cout );
}
