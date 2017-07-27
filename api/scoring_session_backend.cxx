#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <sstream>
#include <json.h>
#include <memory>

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
  ssb_response_t& complete_error( const string& msg );
  ssb_response_t& complete_okay( const string& msg );
  ssb_response_t& add_kv( const string& key, const string& val );
  ssb_response_t& add_kv( const string& key, const domain_handle_type& val );
  void emit_as_json( ostream& os ) const;
private:
  map <string, string> other_kv;
};

//
// symbol table
//

struct ssb_symbol_base_t
{
  enum class s_t { invalid, domain } flavor;
  ssb_symbol_base_t(): flavor( s_t::invalid ) {}
  virtual ~ssb_symbol_base_t() {}
  explicit ssb_symbol_base_t( s_t f ): flavor( f ) {}
};

struct ssb_symbol_domain_t: public ssb_symbol_base_t
{
  domain_handle_type payload;
  ssb_symbol_domain_t( const domain_handle_type& p ): ssb_symbol_base_t( s_t::domain ), payload(p) {}
};


class symbol_table_t
{
public:
  symbol_table_t(): id_counter(0) {}
  size_t add_domain( const domain_handle_type& d)
  {
    size_t id = id_counter++;
    t[id] = std::unique_ptr<ssb_symbol_base_t>( new ssb_symbol_domain_t(d) );
    return id;
  }
private:
  size_t id_counter;
  map< size_t, std::unique_ptr<ssb_symbol_base_t> > t;
};

typedef vector< string > args_t;
typedef function< ssb_response_t ( const args_t&, symbol_table_t&, ssb_response_t& ) > scoring_command_t;

ssb_response_t&
ssb_response_t
::complete_error( const string& msg )
{
  this->add_kv("state", "complete");
  this->add_kv("status","err" );
  this->add_kv( "msg", msg );
  return *this;
}

ssb_response_t&
ssb_response_t
::complete_okay( const string& msg )
{
  this->add_kv("state", "complete");
  this->add_kv("status", "okay" );
  this->add_kv( "msg", msg );
  return *this;
}

ssb_response_t&
ssb_response_t
::add_kv( const string& key, const string& v )
{
  this->other_kv[ key ] = v;
  return *this;
}

ssb_response_t&
ssb_response_t
::add_kv( const string& key, const domain_handle_type& v )
{
  ostringstream oss;
  oss << v;
  this->add_kv( key, oss.str() );
  return *this;
}

void
ssb_response_t
::emit_as_json( ostream& os ) const
{
  JSONNode root(JSON_NODE);
  root.set_name("ssb-response");
  for (auto kv: this->other_kv)
  {
    root.push_back( JSONNode( kv.first, kv.second ));
  }
  os << root.write_formatted() << endl;
  os << "EOD" << endl;
}

ssb_response_t
load_tracks_command( const args_t& args, symbol_table_t& st, ssb_response_t& resp )
{
  if (args.size() != 2) return resp.complete_error( "Incorrect number of arguments" );

  track_handle_list_type tracks;
  if ( ! file_format_manager::read( args[1], tracks )) return resp.complete_error( "Failed to load tracks" );

  handle_list_type h = track_oracle_core::track_to_generic_handle_list( tracks );
  domain_handle_type d = track_oracle_core::create_domain( h );
  size_t id = st.add_domain( d );
  resp.add_kv( "tracks", d );
  ostringstream oss;
  oss << "Loaded " << tracks.size() << " tracks";
  return resp.complete_okay( oss.str() );
}

ssb_response_t
write_tracks_command( const args_t& args, symbol_table_t&, ssb_response_t& resp )
{
  if (args.size() != 3) return resp.complete_error( "Incorrect number of arguments" );

  istringstream iss( args[2] );
  domain_handle_type d;
  if ( ! ( iss >> d )) return resp.complete_error( "Couldn't extract domain handle from '"+args[2]+"'" );

  if ( ! track_oracle_core::is_domain_defined( d )) return resp.complete_error( "No such domain '" + args[2] + "'" );

  handle_list_type h = track_oracle_core::get_domain( d );
  track_handle_list_type tracks = track_oracle_core::generic_to_track_handle_list( h );
  ofstream os( args[1].c_str() );
  if ( ! os ) return resp.complete_error( "Couldn't open '"+args[1]+"' for writing");
  if ( ! track_oracle_core::write_kwiver( os, tracks )) return resp.complete_error( "Couldn't write '"+args[1]+"'" );

  ostringstream oss;
  oss << "Wrote " << tracks.size() << " tracks";
  return resp.complete_okay( oss.str() );
}

ssb_response_t
load_detections_command( const args_t& args, symbol_table_t&, ssb_response_t& resp )
{
  return resp;
}

int main( int argc, char *argv[] )
{
  map< string, scoring_command_t > dispatch_table;
  dispatch_table[ "load_tracks" ] = load_tracks_command;
  dispatch_table[ "load_detections" ] = load_detections_command;
  dispatch_table[ "write_tracks" ] = write_tracks_command;

  {
    ssb_response_t ready;
    ready.add_kv( "state", "ready" );
    ready.emit_as_json( cout );
  }

  string line;
  symbol_table_t symbol_table;

  while ( getline( cin, line ))
  {
    istringstream iss( line );

    ssb_response_t resp;
    resp.add_kv("cmd", line);
    string tmp;

    args_t args;
    while ( (iss >> tmp ))
    {
      args.push_back( tmp );
    }
    if (args.empty())
    {
      resp.add_kv("state", "complete");
      resp.add_kv("status", "ok");
    }
    else
    {
      string key = args[0];
      auto probe = dispatch_table.find( key );
      if (probe == dispatch_table.end())
      {
        resp.complete_error( "unknown keyword '"+key+"'" );
      }
      else
      {
        probe->second( args, symbol_table, resp );
      }
    }
    resp.emit_as_json( cout );
  }
  ssb_response_t done;
  done.add_kv("state", "done");
  done.emit_as_json( cout );
}
