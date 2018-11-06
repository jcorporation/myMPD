var keymap = {
  "shiftKey": {
    "83": {"cmd": "MPD_API_QUEUE_SHUFFLE", "options": []}, // S
    "67": {"cmd": "MPD_API_QUEUE_CROP", "options": []} // C
  },
  "key": {
    "37": {"cmd": "clickPrev", "options": []}, // cursor left
    "39": {"cmd": "clickNext", "options": []}, // cursor right
    "32": {"cmd": "clickPlay", "options": []}, // space
    "83": {"cmd": "clickStop", "options": []}, // s
    "173": {"cmd": "chVolume", "options": [-5]}, // +
    "171": {"cmd": "chVolume", "options": [5]}, // - 
    "67": {"cmd": "MPD_API_QUEUE_CLEAR", "options": []}, // c
    "85": {"cmd": "updateDB", "options": []}, // u
    "82": {"cmd": "rescanDB", "options": []}, // r
    "76": {"cmd": "openLocalPlayer", "options": []}, // l
    "80": {"cmd": "updateSmartPlaylists", "options": []}, // p
    "65": {"cmd": "showAddToPlaylist", "options": ["stream"]}, // a
    "84": {"cmd": "openModal", "options": ["modalSettings"]}, // t
    "89": {"cmd": "openModal", "options": ["modalAbout"]}, // y
    "49": {"cmd": "appGoto", "options": ["Playback"]}, // 1
    "50": {"cmd": "appGoto", "options": ["Queue"]}, // 2
    "51": {"cmd": "appGoto", "options": ["Browse","Database"]}, // 3
    "52": {"cmd": "appGoto", "options": ["Browse","Playlists"]}, // 4
    "53": {"cmd": "appGoto", "options": ["Browse","Filesystem"]}, // 5
    "54": {"cmd": "appGoto", "options": ["Search"]}, // 6
    "77": {"cmd": "openDropdown", "options": ["dropdownMainMenu"]}, // m
    "86": {"cmd": "openDropdown", "options": ["dropdownVolumeMenu"]}, // v
  }
}
