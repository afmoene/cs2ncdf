all:  readme.html readme.rtf readme.txt

readme.html:
	openjade -t sgml -d c:\sgml\sgmlto~1.2\dsssl\html.dsl readme.sgml 

readme.rtf:
	openjade -t rtf -d c:\sgml\docbook\print\docbook.dsl readme.sgml 

readme.txt:
	openjade -t sgml -d c:\sgml\sgmlto~1.2\dsssl\ascii-lynx.dsl readme.sgml > foo.html
	c:\sgml\lynx\lynx -dump -nolist -force_html  foo.html > readme.txt
