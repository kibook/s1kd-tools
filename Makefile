all:
	$(MAKE) -C doc
	$(MAKE) -C tools/s1kd-acronyms
	$(MAKE) -C tools/s1kd-addicn
	$(MAKE) -C tools/s1kd-aspp
	$(MAKE) -C tools/s1kd-brexcheck
	$(MAKE) -C tools/s1kd-checkrefs
	$(MAKE) -C tools/s1kd-defaults
	$(MAKE) -C tools/s1kd-ls
	$(MAKE) -C tools/s1kd-dmrl
	$(MAKE) -C tools/s1kd-flatten
	$(MAKE) -C tools/s1kd-fmgen
	$(MAKE) -C tools/s1kd-icncatalog
	$(MAKE) -C tools/s1kd-index
	$(MAKE) -C tools/s1kd-instance
	$(MAKE) -C tools/s1kd-metadata
	$(MAKE) -C tools/s1kd-neutralize
	$(MAKE) -C tools/s1kd-newcom
	$(MAKE) -C tools/s1kd-newddn
	$(MAKE) -C tools/s1kd-newdm
	$(MAKE) -C tools/s1kd-newdml
	$(MAKE) -C tools/s1kd-newimf
	$(MAKE) -C tools/s1kd-newpm
	$(MAKE) -C tools/s1kd-newupf
	$(MAKE) -C tools/s1kd-ref
	$(MAKE) -C tools/s1kd-refls
	$(MAKE) -C tools/s1kd-syncrefs
	$(MAKE) -C tools/s1kd-transform
	$(MAKE) -C tools/s1kd-upissue
	$(MAKE) -C tools/s1kd-validate

clean:
	$(MAKE) -C doc clean
	$(MAKE) -C tools/s1kd-acronyms clean
	$(MAKE) -C tools/s1kd-addicn clean
	$(MAKE) -C tools/s1kd-aspp clean
	$(MAKE) -C tools/s1kd-brexcheck clean
	$(MAKE) -C tools/s1kd-checkrefs clean
	$(MAKE) -C tools/s1kd-defaults clean
	$(MAKE) -C tools/s1kd-ls clean
	$(MAKE) -C tools/s1kd-dmrl clean
	$(MAKE) -C tools/s1kd-flatten clean
	$(MAKE) -C tools/s1kd-fmgen clean
	$(MAKE) -C tools/s1kd-icncatalog clean
	$(MAKE) -C tools/s1kd-index clean
	$(MAKE) -C tools/s1kd-instance clean
	$(MAKE) -C tools/s1kd-metadata clean
	$(MAKE) -C tools/s1kd-neutralize clean
	$(MAKE) -C tools/s1kd-newcom clean
	$(MAKE) -C tools/s1kd-newddn clean
	$(MAKE) -C tools/s1kd-newdm clean
	$(MAKE) -C tools/s1kd-newdml clean
	$(MAKE) -C tools/s1kd-newimf clean
	$(MAKE) -C tools/s1kd-newpm clean
	$(MAKE) -C tools/s1kd-newupf clean
	$(MAKE) -C tools/s1kd-ref clean
	$(MAKE) -C tools/s1kd-refls clean
	$(MAKE) -C tools/s1kd-syncrefs clean
	$(MAKE) -C tools/s1kd-transform clean
	$(MAKE) -C tools/s1kd-upissue clean
	$(MAKE) -C tools/s1kd-validate clean

install:
	$(MAKE) -C doc install
	$(MAKE) -C tools/s1kd-acronyms install
	$(MAKE) -C tools/s1kd-addicn install
	$(MAKE) -C tools/s1kd-aspp install
	$(MAKE) -C tools/s1kd-brexcheck install
	$(MAKE) -C tools/s1kd-checkrefs install
	$(MAKE) -C tools/s1kd-defaults install
	$(MAKE) -C tools/s1kd-ls install
	$(MAKE) -C tools/s1kd-dmrl install
	$(MAKE) -C tools/s1kd-flatten install
	$(MAKE) -C tools/s1kd-fmgen install
	$(MAKE) -C tools/s1kd-icncatalog install
	$(MAKE) -C tools/s1kd-index install
	$(MAKE) -C tools/s1kd-instance install
	$(MAKE) -C tools/s1kd-metadata install
	$(MAKE) -C tools/s1kd-neutralize install
	$(MAKE) -C tools/s1kd-newcom install
	$(MAKE) -C tools/s1kd-newddn install
	$(MAKE) -C tools/s1kd-newdm install
	$(MAKE) -C tools/s1kd-newdml install
	$(MAKE) -C tools/s1kd-newimf install
	$(MAKE) -C tools/s1kd-newpm install
	$(MAKE) -C tools/s1kd-newupf install
	$(MAKE) -C tools/s1kd-ref install
	$(MAKE) -C tools/s1kd-refls install
	$(MAKE) -C tools/s1kd-syncrefs install
	$(MAKE) -C tools/s1kd-transform install
	$(MAKE) -C tools/s1kd-upissue install
	$(MAKE) -C tools/s1kd-validate install
