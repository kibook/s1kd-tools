targets=doc tools/s1kd-*

all docs clean maintainer-clean install uninstall: $(targets)

$(targets)::
	$(MAKE) -C $@ $(MAKECMDGOALS)

githooks:
	git config --local core.hooksPath .githooks
