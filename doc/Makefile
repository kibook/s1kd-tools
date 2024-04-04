readme_src = DMC-S1KDTOOLS-A-00-00-00-00A-040A-D_EN-CA.XML
intro_src = DMC-S1KDTOOLS-A-00-00-00-00A-018A-D_EN-CA.XML
defaults_src = DMC-S1KDTOOLS-A-00-00-00-00A-014A-D_EN-CA.XML
install_src = DMC-S1KDTOOLS-A-00-00-00-00A-920A-D_EN-CA.XML
example_src = DMC-S1KDTOOLS-A-00-00-00-00A-130A-D_EN-CA.XML
compat_src = DMC-S1KDTOOLS-A-00-00-00-00A-C30A-D_EN-CA.XML
tutorial_src = DMC-S1KDTOOLS-A-00-00-00-00A-018B-D_EN-CA.XML

readme = ../README.md
intro = ../INTRO.md
defaults = ../DEFAULTS.md
install_doc = ../INSTALL.md
example = ../EXAMPLE.md
compat = ../COMPATIBILITY.md
tutorial = ../TUTORIAL.md

s1kd2db = s1kd2db
s1kd2db_flags += -param include.emphasis.role 1

pandoc_md = pandoc -f docbook -t gfm -s
pandoc_md_toc = pandoc -f docbook -t markdown | pandoc -f markdown -t gfm -s --toc
pandoc_man = pandoc -f docbook -t man -s

prefix = /usr/local
datarootdir = $(prefix)/share
mandir = $(datarootdir)/man
man1dir = $(mandir)/man1
man5dir = $(mandir)/man5

INSTALL = install -D

all:

docs: s1kd-tools.1 s1kd-defaults.5 $(readme) $(intro) $(defaults) $(install_doc) $(example) $(compat) $(tutorial)

$(readme): $(readme_src) .icncatalog
	s1kd-icncatalog -m github $(readme_src) | $(s1kd2db) - $(s1kd2db_flags) | $(pandoc_md) -o $(readme)

$(intro): $(intro_src)
	$(s1kd2db) $(intro_src) $(s1kd2db_flags) | $(pandoc_md) -o $(intro)

$(defaults): $(defaults_src)
	$(s1kd2db) $(defaults_src) $(s1kd2db_flags) | $(pandoc_md) -o $(defaults)

$(install_doc): $(install_src)
	$(s1kd2db) $(install_src) $(s1kd2db_flags) | $(pandoc_md) -o $(install_doc)

$(example): $(example_src) .icncatalog
	s1kd-icncatalog -m github $(example_src) | $(s1kd2db) - $(s1kd2db_flags) | $(pandoc_md_toc) -o $(example)

$(compat): $(compat_src)
	$(s1kd2db) $(compat_src) $(s1kd2db_flags) | $(pandoc_md) -o $(compat)

$(tutorial): $(tutorial_src) .icncatalog ICN-S1KDTOOLS-A-000000-A-KHZAE-00003-A-001-01.PNG
	s1kd-icncatalog -m github $(tutorial_src) | $(s1kd2db) - $(s1kd2db_flags) | $(pandoc_md_toc) -o $(tutorial)

ICN-S1KDTOOLS-A-000000-A-KHZAE-00003-A-001-01.PNG: ICN-S1KDTOOLS-A-000000-A-KHZAE-00003-A-001-01.DOT
	dot -Tpng -Gdpi=150 $< > $@

s1kd-tools.1: $(intro_src)
	$(s1kd2db) $(intro_src) $(s1kd2db_flags) | $(pandoc_man) -o $@

s1kd-defaults.5: $(defaults_src)
	$(s1kd2db) $(defaults_src) $(s1kd2db_flags) | $(pandoc_man) -o $@

.PHONY: docs clean maintainer-clean install uninstall

clean:

maintainer-clean:
	$(RM) s1kd-tools.1 s1kd-defaults.5 $(readme) $(intro) $(defaults) $(install_doc) $(example) $(compat)

install:
	$(INSTALL) s1kd-tools.1 $(man1dir)/s1kd-tools.1
	$(INSTALL) s1kd-defaults.5 $(man5dir)/s1kd-defaults.5

uninstall:
	$(RM) $(man1dir)/s1kd-tools.1
	$(RM) $(man5dir)/s1kd-defaults.5
