#! /bin/ruby

require 'socket'
# SERVER_NAME is the name of the server
# and PORT is the port number specified in server/delchef.c
s = TCPSocket.new(SERVER_NAME,PORT)
s.close
