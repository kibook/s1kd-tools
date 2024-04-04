.PHONY: all install uninstall

prefix = /usr/local
datarootdir = $(prefix)/share

install_prefix = $(datarootdir)/xml/schema/s1kd-tools

INSTALL = install

schemas = *.xsd

xml_catalog_file=/etc/xml/catalog

orig_schema_url=http://khzae.net/x/s1000d/s1kd-tools/schema
repl_schema_url=file://$(install_prefix)

all:

install:
	mkdir -p $(install_prefix)
	$(INSTALL) $(schemas) $(install_prefix)
	xmlcatalog --noout --add rewriteURI '$(orig_schema_url)' '$(repl_schema_url)' $(xml_catalog_file)

uninstall:
	rm -rf $(install_prefix)
	xmlcatalog --noout --del '$(orig_schema_url)' $(xml_catalog_file)
