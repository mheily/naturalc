# FIXME - this is somewhat broken

all:	clean
	cd ../docbook && make
	cd ../.. && make doxygen
	cp -R ../doxygen .
	cp -R ../docbook/html docbook
	cp ../../README readme.txt
	cp ../../BUGS bugs.txt

clean:
	rm -rf doxygen/ docbook/ readme.txt bugs.txt

sync:
	rsync -avz ./* $$WWWHOST:/var/www/naturalc.org
