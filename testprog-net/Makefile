# References: 
# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
# http://nuclear.mutantstargoat.com/articles/make/

CC=gcc
PREFIX = /usr

testprog-net: testprog-net.c
	$(CC) -o testprog-net testprog-net.c

clean:
	rm -f testprog-net

install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp testprog-net $(DESTDIR)$(PREFIX)/bin/testprog-net
	cp testprog-net.conf /etc/testprog-net.conf
	mkdir -p /var/testprog-net
	cp testprog-net.service /etc/systemd/system/testprog-net.service

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/testprog-net
	rm -f /etc/testprog-net.conf
	rm -f /etc/systemd/system/testprog-net.service
	rm -rf /var/testprog-net
		
