# $Id$
# Your commands:
RM=rm
JADE=openjade
LYNX=c:\sgml\lynx\lynx

# Style sheets
HTMLDSL=c:\sgml\sgmlto~1.2\dsssl\html.dsl
PRINTDSL=c:\sgml\docbook\print\docbook.dsl
TXTDSL=c:\sgml\sgmlto~1.2\dsssl\ascii-lynx.dsl

all:  readme.html readme.rtf readme.txt

readme.html: readme.sgml
	$(JADE) -t sgml -d $(HTMLDSL) readme.sgml 

readme.rtf: readme.sgml
	$(JADE) -t rtf -d $(PRINTDSL) readme.sgml 

readme.txt: readme.sgml
	$(JADE) -t sgml -d $(TXTDSL) readme.sgml > foo.html
	$(LYNX) -dump -width=90 -nolist -force_html  foo.html > readme.txt
	$(RM) foo.html
