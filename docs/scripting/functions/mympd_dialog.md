---
layout: page
permalink: /scripting/functions/mympd_dialog
title: Accessing the myMPD API
---

Returns an Jsonrpc response for a script dialog.

```lua
local title = "Script title"
local data = {
  { name = "testinput", type = "text", value = "testvalue" },
  { name = "testpassword", type = "password", value = "" },
  { name = "testcheckbox", type = "checkbox", value = false },
  { name = "testradio", type = "radio", value = { "radio1", "radio2" }, defaultValue = "radio2" },
  { name = "testselect", type = "select", value = { "option1", "option2" }, defaultValue = "option1" },
  { name = "testlist", type = "list", value = { "val1", "val2", "val3" }, defaultValue = "" }
}
local callback = "testscript"
return mympd.dialog(title, data, callback)
```

**Parameters:**

| PARAMETER | TYPE | DESCRIPTION |
| --------- | ---- | ----------- |
| title | string | Dialog title |
| data | table | The dialog definition. |
| callback | string | Script to call for the submit button |
{: .table .table-sm }

**Returns:**

A Jsonrpc string with method `script_dialog`.

## Dialog definition

| KEY | DESCRIPTION |
| --- | ----------- |
| name | Name of the form element. |
| type | Type of the form element. |
| value | The value(s) of the form element. |
| defaultValue | The defaultValue of the form element. |
{: .table .table-sm }

| TYPE | DESCRIPTION |
| ---- | ----------- |
| text | Text input field. |
| password | Password input field. |
| checkbox | Checkbox, value is true if checked, else false. |
| select | Selectbox with multiple options. |
| radio | Radios |
| list | List of elements to select from. Selected items are separated by `;;` |
{: .table .table-sm }

These are the same types as for script arguments.
