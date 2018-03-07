# SimpleSecureChat
A simple program written in C mainly using libOpenSSL for a Simple, yet Secure chat.

PRE-ALPHA

Requirements:
1. openssl-dev (sudo yum install openssl-devel || sudo apt-get install libopenssl-dev) 
2. gcc (sudo yum install gcc || sudo apt-get install gcc)
3. binn (Data Serialization Library) available on github

To compile:

    Use makescript.sh in client folder

Debugging with ncat: 

    server# ncat --ssl -klvp 5050 --ssl-key key.pem --ssl-cert cert.pem 


Command to Create Server Certificate 

    server# openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 

