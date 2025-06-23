---
title: mympd-script
---

`mympd-script` is a small command line tool to execute myMPD scripts.

`Key=Value` parameters can be used to fill the arguments table in the Lua script.

## Call available script (test.lua)

mympd-script can call existing scripts. This API call is not restricted by the `scripts_external` and `scriptacl` options and uses the standard `/api/<partition>` API endpoint.

```sh
mympd-script https://localhost default test key1=value1 
```

## Script from STDIN

It reads the script from STDIN and submits it to myMPD for execution.

```sh
mympd-script https://localhost:8443 default - key1=value1 <<< 'print arguments["key1"]'
```

For security reasons this function is disabled by default. You can enable it by setting the configuration file `scripts_external` to `true`. The API-Endpoint is restricted to localhost. This can be configured with the `scriptacl` option in the config folder.

The API endpoint for this function is: `/script-api/<partition>`
