all:  readme.html readme.rtf readme.txt

readme.html: readme.sgml
	openjade -t sgml -d c:\sgml\sgmlto~1.2\dsssl\html.dsl readme.sgml 

readme.rtf: readme.sgml
	openjade -t rtf -d c:\sgml\docbook\print\docbook.dsl readme.sgml 

readme.txt: readme.sgml
	openjade -t sgml -d c:\sgml\sgmlto~1.2\dsssl\ascii-lynx.dsl readme.sgml > foo.html
	c:\sgml\lynx\lynx -dump -width=90 -nolist -force_html  foo.html > readme.txt
