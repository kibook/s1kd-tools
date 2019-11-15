.PHONY: all install uninstall

PREFIX=/usr/local
INSTALL_PREFIX=$(PREFIX)/share/xml/schema/s1kd-tools
INSTALL=install

SCHEMAS=*.xsd

XML_CATALOG_FILE=/etc/xml/catalog
ORIG_SCHEMA_URL=http://khzae.net/x/s1000d/s1kd-tools/schema
REPL_SCHEMA_URL=file://$(INSTALL_PREFIX)

all:

install:
	mkdir -p $(INSTALL_PREFIX)
	$(INSTALL) $(SCHEMAS) $(INSTALL_PREFIX)
	xmlcatalog --noout --add rewriteURI '$(ORIG_SCHEMA_URL)' '$(REPL_SCHEMA_URL)' $(XML_CATALOG_FILE)

uninstall:
	rm -rf $(INSTALL_PREFIX)
	xmlcatalog --noout --del '$(ORIG_SCHEMA_URL)' $(XML_CATALOG_FILE)
