TARGETS=doc tools/s1kd-*

all docs clean maintainer-clean install uninstall: $(TARGETS)

$(TARGETS)::
	$(MAKE) -C $@ $(MAKECMDGOALS)
