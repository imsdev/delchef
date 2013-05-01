delchef
=======

This is a service written in C that chef clients can connect to to automatically delete themselves from the chef-server.
Any client that connects to the server will have DNS run against their IP address, and the server will run
  "knife client delete " + DNS_NAME + " -y"
By giving client names the DNS names of workstations in your enterprise, this will effectively solve the problem of
having to manually delete clients when wiping workstations, as long as you make running the client script part of your
deployment.
