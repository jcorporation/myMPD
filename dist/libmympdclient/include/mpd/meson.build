version_conf = configuration_data()
version_conf.set('MAJOR_VERSION', splitted_version[0])
version_conf.set('MINOR_VERSION', splitted_version[1])
if splitted_version.length() >= 3
  version_conf.set('PATCH_VERSION', splitted_version[2])
else
  version_conf.set('PATCH_VERSION', '0')
endif
version_h = configure_file(input: 'version.h.in', output: 'version.h', configuration: version_conf)

install_headers(
  'async.h',
  'audio_format.h',
  'client.h',
  'capabilities.h',
  'compiler.h',
  'connection.h',
  'database.h',
  'directory.h',
  'entity.h',
  'error.h',
  'feature.h',
  'fingerprint.h',
  'idle.h',
  'list.h',
  'mixer.h',
  'mount.h',
  'neighbor.h',
  'parser.h',
  'partition.h',
  'password.h',
  'player.h',
  'playlist.h',
  'position.h',
  'protocol.h',
  'queue.h',
  'recv.h',
  'replay_gain.h',
  'response.h',
  'send.h',
  'status.h',
  'stats.h',
  'tag.h',
  'output.h',
  'pair.h',
  'search.h',
  'socket.h',
  'song.h',
  'sticker.h',
  'settings.h',
  'message.h',
  'binary.h',
  'albumart.h',
  'readpicture.h',
  version_h,
  subdir: 'mpd',
)
