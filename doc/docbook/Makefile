#	$Id: $
#
# Common DocBook Makefile
# (copied from the Recvmail source tree)
#
# NOTE:
#
#    If you include this file, you *must* define the BOOK and VERSION variables
#

BOOK=		naturalc
VERSION=	0.1
LINT		= xmllint --noout --postvalid --xinclude 

#For OpenBSD:
#CC=		SGML_CATALOG_FILES=/usr/local/share/sgml/catalog openjade 
#CFLAGS=		-t xml \
#	-d /usr/local/share/sgml/docbook/dsssl/modular/html/docbook.dsl \
#	/usr/local/share/sgml/docbook/dsssl/modular/dtds/decls/xml.dcl 

# For Linux:
CC=		xmlto

all:	clean xmlto
	
clean:
	rm -rf html
	rm -f *.pdf *.ps *.htm *~ *.BAK

test:
	echo "FIXME - check for xmllint(1)"
#$(LINT) $(BOOK).xml"

xmlto:
	mkdir -p html
	xmlto -o html html-nochunks $(BOOK).xml
	xmlto -o html html $(BOOK).xml
#xmlto pdf $(BOOK).xml
#	xmlto ps $(BOOK).xml

openbsd-single-html:
	$(CC) -V nochunks $(CFLAGS) $(BOOK).xml > $(BOOK).html || true

openbsd-multi-html:
	$(CC) $(CFLAGS) $(BOOK).xml || true

install-prereq:

# For Debian
	sudo apt-get install docbook libxml2-utils xmlto passivetex

# For OpenBSD
	pkg_add libxml openjade docbook-dsssl

# Fake installation target
distdir:
	true
