# References: 
# http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
# http://nuclear.mutantstargoat.com/articles/make/

CC=gcc
PREFIX = /usr

testprog: testprog.c
	$(CC) -o testprog testprog.c

clean:
	rm -f testprog

install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp testprog $(DESTDIR)$(PREFIX)/bin/testprog
	cp testprog.conf /etc/testprog.conf
	mkdir -p /var/testprog
	cp testprog.service /etc/systemd/system/testprog.service

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/testprog
	rm -f /etc/testprog.conf
	rm -f /etc/systemd/system/testprog.service
	rm -rf /var/testprog
		
