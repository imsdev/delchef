CC=gcc

all:
	$(CC) -o /usr/bin/delchef delchef.c
	userdel delchef
	useradd --no-create-home --user-group --inactive -1 --system --shell "/sbin/nologin" delchef
	if ! [ -d /var/log/delchef]; then \
	  mkdir /var/log/delchef; \
	fi
	mkdir /var/log/delchef
	touch /var/log/delchef/delchef.log
	chmod -R 0664 /var/log/delchef
	chown -R delchef:delchef /var/log/delchef
	chmod 0555 /usr/bin/delchef
	chmod u+s /usr/bin/delchef
	chmod g+s /usr/bin/delchef
	chown delchef:delchef /usr/bin/delchef
	rm -rf /etc/init/delchef
	mv delchef.upstart /etc/init/delchef.conf
	chmod /etc/init/delchef.conf 0644
	chown root:root /etc/init/delchef.conf