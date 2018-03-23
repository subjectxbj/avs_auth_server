# avs_auth_server

The authentication server for avs-device-sdk project is implemented by python. The total size of python2.7 and required packages are almost 50MB.
This is too expensive for an embled device.

So this project will use ANSI c to implement similar function.

This application has dependency on following items: curl and json-c.

So before build this project, need to build and install these libraries:


    wget https://github.com/curl/curl/releases/download/curl-7_59_0/curl-7.59.0.tar.gz
    tar -xzf curl-7.59.0.tar.gz
    cd curl-7.59.0
    ./configure
    make
    make install

    git clone https://github.com/json-c/json-c.git
    cd json-c 
    ./autogen.sh
    ./configure
    make 
    make install
    ldconfig

By default, these libraries and include files are installed under /usr/local

Then you can build avs_auth_server
    
    make
    make install 

   
