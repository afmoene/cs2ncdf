# $Id$
# Your remove command might be platform dependent
RM=rm

# Your jade command
JADE=openjade

# Your lynx command
LYNX=c:\sgml\lynx\lynx


all:  readme.html readme.rtf readme.txt

readme.html: readme.sgml
	$(JADE) -t sgml -d c:\sgml\sgmlto~1.2\dsssl\html.dsl readme.sgml 

readme.rtf: readme.sgml
	$(JADE) -t rtf -d c:\sgml\docbook\print\docbook.dsl readme.sgml 

readme.txt: readme.sgml
	$(JADE) -t sgml -d c:\sgml\sgmlto~1.2\dsssl\ascii-lynx.dsl readme.sgml > foo.html
	$(LYNX) -dump -width=90 -nolist -force_html  foo.html > readme.txt
	$(RM) foo.html
