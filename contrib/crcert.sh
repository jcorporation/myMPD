#!/bin/sh

[ -d /etc/mympd/ssl ] && rm -r /etc/mympd/ssl
mkdir -p /etc/mympd/ssl/ca/certs
cd /etc/mympd/ssl/ca

echo '01' > serial
touch index.txt
touch index.txt.attr

echo "Creating ca"

cat > ca.cnf << EOL
[req]
distinguished_name = root_ca_distinguished_name
x509_extensions = root_ca_extensions
prompt = no

[root_ca_distinguished_name]
O = myMPD
CN = myMPD_CA

[root_ca_extensions]
basicConstraints = CA:true

[ ca ]
default_ca = mympd_ca

[mympd_ca]
dir = /etc/mympd/ssl/ca
database = /etc/mympd/ssl/ca/index.txt
new_certs_dir = /etc/mympd/ssl/ca/certs/
serial = /etc/mympd/ssl/ca/serial
copy_extensions = copy
policy = local_ca_policy
x509_extensions = local_ca_extensions
default_md = sha256

[ local_ca_policy ]
commonName = supplied
organizationName = supplied

[ local_ca_extensions ]
basicConstraints = CA:false

EOL

openssl req -new -x509 -newkey rsa:2048 -sha256 -days 1000 -nodes -config ca.cnf \
	-keyout ca.key -out ca.pem

HOSTNAME=$(hostname)
FQDN=$(hostname -f)
IP=$(getent hosts $HOSTNAME | awk {'print $1'})

cd /etc/mympd/ssl
echo "Creating cert:"
echo "\t$HOSTNAME"
echo "\t$FQDN"
echo "\t$IP"

cat > req.cnf << EOL
[req]
distinguished_name = req_distinguished_name
req_extensions = v3_req
prompt = no

[req_distinguished_name]
O = myMPD
CN = $FQDN

[v3_req]
basicConstraints = CA:FALSE
keyUsage = digitalSignature, keyEncipherment, dataEncipherment
extendedKeyUsage = serverAuth
subjectAltName = @alt_names

[alt_names]
DNS.1 = $HOSTNAME
DNS.2 = $FQDN
DNS.3 = localhost
IP.1 = $IP
IP.2 = 127.0.0.1
EOL

openssl req -new -sha256 -newkey rsa:2048 -days 1000 -nodes -config req.cnf \
	-keyout server.key -out server.csr \
	-extensions v3_req

echo "Sign cert with ca"
openssl ca -in server.csr -cert ca/ca.pem -keyfile ca/ca.key -config ca/ca.cnf \
	-out server.pem -days 1000 -batch

rm server.csr
rm ca/ca.cnf
rm req.cnf
