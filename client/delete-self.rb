#! /bin/ruby

require 'socket'
s = TCPSocket.new(SERVER_NAME,62085)
s.close