.PHONY: all install uninstall

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/share/xml/schema/s1kd-tools
INSTALL=install -D

SCHEMAS=*.xsd

XML_CATALOG_FILE=/etc/xml/catalog
ORIG_SCHEMA_URL=http://khzae.net/x/s1000d/s1kd-tools/schema
REPL_SCHEMA_URL=file://$(INSTALL_PREFIX)

all:

install:
	for schema in $(SCHEMAS); do $(INSTALL) "$$schema" $(INSTALL_PREFIX)/"$$schema"; done
	xmlcatalog --noout --add rewriteURI '$(ORIG_SCHEMA_URL)' '$(REPL_SCHEMA_URL)' $(XML_CATALOG_FILE)

uninstall:
	rm -rf $(INSTALL_PREFIX)
