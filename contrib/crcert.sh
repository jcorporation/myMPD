#!/bin/sh

HOSTNAME=$(hostname)
FQDN=$(hostname -f)
IP=$(getent hosts $HOSTNAME | awk {'print $1'})

echo "Creating cert:"
echo "\t$HOSTNAME"
echo "\t$FQDN"
echo "\t$IP"

cat > /etc/mympd/openssl.cnf << EOL
[req]
distinguished_name = req_distinguished_name
x509_extensions = v3_req
prompt = no

[req_distinguished_name]
O = myMPD
CN = $FQDN

[v3_req]
keyUsage = keyEncipherment, dataEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[alt_names]
DNS.1 = $HOSTNAME
DNS.2 = $FQDN
IP.1 = $IP
EOL

openssl req -x509 -sha256 -newkey rsa:2048 -days 1000 -nodes -config /etc/mympd/openssl.cnf\
	-keyout /etc/mympd/server.key -out /etc/mympd/server.pem \
	-extensions 'v3_req'

