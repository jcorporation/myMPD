---
title: mympd-script
---

`mympd-script` is a small command line tool to execute myMPD scripts.

`Key=Value` parameters can be used to fill the arguments table in the Lua script.

**Script from STDIN:**

It reads the script from STDIN and submits it to myMPD for execution.

```sh
mympd-script https://localhost default - key1=value1 <<< 'print arguments["key1"]'
```

For security reasons this function is restricted to localhost. This can be configured with the `scriptacl` option in the config folder.

The API endpoint for this function is: `/script-api/<partition>`

**Call available script (test.lua):**

mympd-script can also call existing scripts. This API call is not restricted by the `scriptacl` option and uses the standard `/api/<partition>` API endpoint.

```sh
mympd-script https://localhost default test key1=value1 
```
