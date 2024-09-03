#================================================================
# Makefile for DXMake program maintenance utility package.
#
# (C) Copyright 1985-1988, 1990, 1992 Ammon R. Campbell.
# All rights reserved.
#
# This source code contains trade secrets of the author and
# may not be disclosed without the written consent of the
# author.
#
# Default targets:
#	make.exe	Make utility for MS-DOS or Windows
#	make.txt	Documentation for make.exe
#	touch.exe	Touch utility for MS-DOS or Windows
#	touch.txt	Documentation for touch.exe
#	dxmake.hlp	Windows on-line help
#================================================================

#----------------------------------------------------------------
# Macros:
#----------------------------------------------------------------
RELDATE = 05-02-92
RELTIME = 20:15
CC = cl
CCFLAGSDOS = -c -AS -Od -Gs -W3 -nologo
CCFLAGSWIN = -c -AS -Od -Gsw -W3 -nologo -DWIN
# For codeview:
# CCFLAGSDOS = -c -AS -Od -Gs -W3 -nologo -Zi
# CCFLAGSWIN = -c -AS -Od -Gsw -W3 -nologo -DWIN -Zi
RC = rc
RCFLAGS1 = -r
RCFLAGS2 = -t -30
LINK = link
LFLAGS = /nologo /stack:4096
LFLAGSW = /nologo /stack:4096 /NOD
# For codeview:
# LFLAGS = /nologo /stack:4096 /CODEVIEW
# LFLAGSW = /nologo /stack:4096 /NOD /CODEVIEW
WLIBS = libw slibcew oldnames
HC = hc

#----------------------------------------------------------------
# Inference rules.
#----------------------------------------------------------------

.SUFFIXES:
.SUFFIXES:	.obj .obw .c .res .rc .hlp .rtf

.c.obj:
	$(CC) $(CCFLAGSDOS) $<

.c.obw:
	$(CC) $(CCFLAGSWIN) -Fo$*.obw $<

.rtf.hlp:
	if exist $*.ph del $*.ph
	$(HC) $*.hpj

.rc.res:
	$(RC) $(RCFLAGS1) $<

#----------------------------------------------------------------
# Master make dependency.
#----------------------------------------------------------------

all:	make.exe make.txt \
	touch.exe touch.txt \
	dxmake.hlp

#----------------------------------------------------------------
# Miscellaneous targets.
#----------------------------------------------------------------

touch:	touch.exe
	touch -t$(RELTIME) -d$(RELDATE) *

zip:
	if exist dxmake.zip del dxmake.zip
	pkzip -a dxmake.zip *.*

oldrelease:	make.exe make.txt make.inf \
		touch.exe touch.txt \
		dxmake.hlp readme
	if exist dxm150.zip del dxm150.zip
	pkzip -a dxm150.zip readme make.inf make.txt touch.txt
	pkzip -a dxm150.zip make.exe touch.exe dxmake.hlp

release:	make.exe make.txt make.inf \
		touch.exe touch.txt \
		dxmake.hlp readme
	if exist dxm150.exe del dxm150.exe
	if exist dx.inf del dx.inf
	echo "# dx.inf"				> dx.inf
	echo outfile=dxm150.exe			>> dx.inf
	echo appname=DXMAKE version 1.50	>> dx.inf
	echo message1=				>> dx.inf
	echo message2=				>> dx.inf
	echo file1=readme			>> dx.inf
	echo file2=make.inf			>> dx.inf
	echo file3=make.txt			>> dx.inf
	echo file4=touch.txt			>> dx.inf
	echo file5=make.exe			>> dx.inf
	echo file6=touch.exe			>> dx.inf
	echo file7=dxmake.hlp			>> dx.inf
	buildx dx.inf

bogus:
	test 0 -eq 1
	test 1 -eq 1
	hoseme

#----------------------------------------------------------------
# Target used to remove all targets so the next build will be
# clean.
#----------------------------------------------------------------

clean:
	if exist *.obj del *.obj
	if exist *.obw del *.obw
	if exist *.exe del *.exe
	if exist *.res del *.res
	if exist *.map del *.map
	if exist *.lnk del *.lnk
	if exist make.txt del make.txt
	if exist touch.txt del touch.txt

#----------------------------------------------------------------
# Build document files.
#----------------------------------------------------------------

make.txt:	make.1
	nroff make.1 > make.txt

touch.txt:	touch.1
	nroff touch.1 > touch.txt

#----------------------------------------------------------------
# Build on-line help for Windows.
#----------------------------------------------------------------

dxmake.hlp:	dxmake.rtf dxmake.hpj

#----------------------------------------------------------------
# Build common MS-DOS object files.
#----------------------------------------------------------------

cvtslash.obj:	cvtslash.c cvtslash.h

fnexp.obj:	fnexp.c fnexp.h

getpath.obj:	getpath.c getpath.h

wild.obj:	wild.c wild.h getpath.h fnexp.h

#----------------------------------------------------------------
# Build common Windows object files.
#----------------------------------------------------------------

cvtslash.obw:	cvtslash.c cvtslash.h

fnexp.obw:	fnexp.c fnexp.h

getpath.obw:	getpath.c getpath.h

wild.obw:	wild.c wild.h getpath.h fnexp.h

#----------------------------------------------------------------
# Build 'make_st' utility for MS-DOS.  This file gets bound
# into 'make.exe', which is a dual mode executable.
#----------------------------------------------------------------

make_st.exe:	make.obj makebld.obj		\
		makein.obj makemac.obj		\
		makemem.obj			\
		makerul.obj maketar.obj		\
		makeprec.obj makesuf.obj	\
		makexpnd.obj makeutil.obj	\
		make_st.lnk			\
		wild.obj fnexp.obj getpath.obj cvtslash.obj
	$(LINK) @make_st.lnk;

make_st.lnk:	makefile
	echo $(LFLAGS) make makebld+		> make_st.lnk
	echo makein makemac makemem makerul+	>> make_st.lnk
	echo maketar+				>> make_st.lnk
	echo makeprec makesuf makexpnd+		>> make_st.lnk
	echo makeutil+				>> make_st.lnk
	echo wild fnexp getpath cvtslash	>> make_st.lnk
	echo make_st.exe;			>> make_st.lnk

make.obj:	make.c make.h makemsg.h cvtslash.h

makebld.obj:	makebld.c make.h makemsg.h

makein.obj:	makein.c make.h makemsg.h

makemac.obj:	makemac.c make.h makemsg.h

makemem.obj:	makemem.c make.h makemsg.h

makeprec.obj:	makeprec.c make.h makemsg.h

makerul.obj:	makerul.c make.h makemsg.h

makesuf.obj:	makesuf.c make.h makemsg.h

maketar.obj:	maketar.c make.h makemsg.h

makeutil.obj:	makeutil.c make.h makemsg.h

makexpnd.obj:	makexpnd.c make.h makemsg.h

#----------------------------------------------------------------
# Build 'make' utility for Microsoft Windows.
#----------------------------------------------------------------

make.exe:	make.obw makew.obw makebld.obw	\
		makein.obw makemac.obw		\
		makemem.obw			\
		makerul.obw maketar.obw		\
		makeprec.obw makesuf.obw	\
		makexpnd.obw makeutil.obw	\
		make.lnk make_st.exe		\
		wild.obw fnexp.obw getpath.obw cvtslash.obw \
		make.res make.def
	$(LINK) @make.lnk;
	$(RC) $(RCFLAGS2) $*.res

make.lnk:	makefile
	echo $(LFLAGSW) /nologo /stack:4096 make.obw makew.obw + > make.lnk
	echo makebld.obw makein.obw makemac.obw +		>> make.lnk
	echo makemem.obw +					>> make.lnk
	echo makerul.obw maketar.obw makeprec.obw +		>> make.lnk
	echo makesuf.obw makexpnd.obw makeutil.obw +		>> make.lnk
	echo wild.obw fnexp.obw getpath.obw cvtslash.obw	>> make.lnk
	echo $*.exe,, $(WLIBS), $*.def;				>> make.lnk

make.res:	make.rc makemsg.h makecid.h make.ico

make.obw:	make.c make.h makemsg.h cvtslash.h

makew.obw:	makew.c make.h makemsg.h makecid.h

makebld.obw:	makebld.c make.h makemsg.h

makein.obw:	makein.c make.h makemsg.h

makemac.obw:	makemac.c make.h makemsg.h

makemem.obw:	makemem.c make.h makemsg.h

makeprec.obw:	makeprec.c make.h makemsg.h

makerul.obw:	makerul.c make.h makemsg.h

makesuf.obw:	makesuf.c make.h makemsg.h

maketar.obw:	maketar.c make.h makemsg.h

makeutil.obw:	makeutil.c make.h makemsg.h

makexpnd.obw:	makexpnd.c make.h makemsg.h

#----------------------------------------------------------------
# Build 'touch_st' utility for MS-DOS.  This file gets bound
# into 'touch.exe', which is a dual mode executable.
#----------------------------------------------------------------

touch_st.exe:	touch.obj wild.obj fnexp.obj getpath.obj cvtslash.obj
	$(LINK) $(LFLAGS) $**, $@;

touch.obj:	touch.c touchmsg.h getpath.h fnexp.h wild.h cvtslash.h

#----------------------------------------------------------------
# Build 'touch' utility for Microsoft Windows.
#----------------------------------------------------------------

touch.exe:	touch.obw wild.obw fnexp.obw getpath.obw cvtslash.obw \
		touch.res touch.def touch_st.exe touch.lnk
	$(LINK) @touch.lnk;
	$(RC) $(RCFLAGS2) $*.res

touch.lnk:	makefile
	echo $(LFLAGSW) touch.obw wild.obw + > touch.lnk
	echo fnexp.obw getpath.obw + >> touch.lnk
	echo cvtslash.obw, $*.exe,, $(WLIBS), touch.def; >> touch.lnk

touch.res:	touch.rc touch.ico touchmsg.h touchcid.h

touch.obw:	touch.c touchmsg.h touchcid.h getpath.h \
		fnexp.h wild.h cvtslash.h

#----------------------------------------------------------------
# End makefile.
#----------------------------------------------------------------
