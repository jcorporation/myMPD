---
layout: page
permalink: /configuration/acl
title: ACL
---

myMPD supports simple IP ACLs to restrict connections to the webserver and to the remote scripting API endpoint.

The ACL is a comma separated list of IPv4 subnets: x.x.x.x/x Each subnet is prepended by either a - or a + sign. A plus sign means allow, where a minus sign means deny.

If the acl is empty, all connections are allowed else all connections are denied if not explicitly allowed.

<div class="alert alert-warning">
ACLs for IPv6 are currently not supported.
</div>

## Example ACLs

| ACL | DESCRIPTION |
| --- | ----------- |
| `+0.0.0.0/0`| Allow all |
| `+127.0.0.0/8`| Allow localhost |
| `+127.0.0.0/8,+192.168.0.0/24` | Allow localhost and all hosts in the net 192.168.0.x |
{: .table .table-sm}
