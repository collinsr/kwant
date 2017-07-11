kwiver_add_executable( scoring_session_backend scoring_session_backend.cxx )

# todo: flag track_oracle, json as mandatory

target_link_libraries(
  scoring_session_backend
  track_oracle
  track_oracle_file_formats
  ${LIBJSON_LIBRARY}
)
