#!/bin/sh

VARDIR=$1
if [ "$VARDIR" = "" ]
then
  VARDIR="/var/lib/mympd/ssl"
fi

if [ -d ${VARDIR} ]
then
  echo "SSL directory exists, to recreate certificates: \"rm -r ${VARDIR}\""
  exit 0
fi

mkdir -p ${VARDIR}/ca/certs
chmod 700 ${VARDIR}
cd ${VARDIR}/ca || exit 1

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
dir = ${VARDIR}/ca
database = ${VARDIR}/ca/index.txt
new_certs_dir = ${VARDIR}/ca/certs/
serial = ${VARDIR}/ca/serial
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

openssl req -new -x509 -newkey rsa:2048 -sha256 -days 3650 -nodes -config ca.cnf \
	-keyout ca.key -out ca.pem

HOSTNAME=$(hostname)
FQDN=$(hostname -f)

cd ${VARDIR} || exit 1

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
IP.1 = 127.0.0.1
EOL

I=2
getent hosts "$HOSTNAME" | awk '{print $1}' | while read -r line
do
    echo "IP.${I} = ${line}" >> req.cnf
    I=$((I+1))
done

openssl req -new -sha256 -newkey rsa:2048 -nodes -config req.cnf \
	-keyout server.key -out server.csr \
	-extensions v3_req

echo "Sign cert with ca"
openssl ca -in server.csr -cert ca/ca.pem -keyfile ca/ca.key -config ca/ca.cnf \
	-out server.pem -days 3650 -batch

rm server.csr

chown -R mympd.mympd ${VARDIR}
