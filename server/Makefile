CC=gcc

all:
	$(CC) -o /usr/bin/delchef delchef.c
	useradd --no-create-home --user-group --inactive -1 --system --shell "/sbin/nologin" delchef
	mkdir /var/log/delchef
	touch /var/log/delchef/delchef.log
	chmod -R 0664 /var/log/delchef
	chown -R delchef:delchef /var/log/delchef
	chmod 0555 /usr/bin/delchef
	chmod u+s /usr/bin/delchef
	chmod g+s /usr/bin/delchef
	chown delchef:delchef /usr/bin/delchef