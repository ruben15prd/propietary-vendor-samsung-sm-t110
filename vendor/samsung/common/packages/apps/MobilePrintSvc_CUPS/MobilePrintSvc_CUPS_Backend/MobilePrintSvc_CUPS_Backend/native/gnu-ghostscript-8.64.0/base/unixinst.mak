#  Copyright (C) 2001-2006 Artifex Software, Inc.
#  All Rights Reserved.
#
#  This file is part of GNU ghostscript
#
#  GNU ghostscript is free software; you can redistribute it and/or modify it under
#  the terms of the version 2 of the GNU General Public License as published by the Free Software
#  Foundation.
#
#  GNU ghostscript is distributed in the hope that it will be useful, but WITHOUT
#  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
#  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along with
#  ghostscript; see the file COPYING. If not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# $Id: unixinst.mak,v 1.11 2007/09/11 15:24:07 Arabidopsis Exp $
# Partial makefile common to all Unix and Desqview/X configurations,
# containing the `install' targets.
# This is the very last part of the makefile for these configurations.

install: install-exec install-scripts install-data $(INSTALL_SHARED) $(INSTALL_CONTRIB)

# The sh -c in the rules below is required because Ultrix's implementation
# of sh -e terminates execution of a command if any error occurs, even if
# the command traps the error with ||.

# We include mkdirs for datadir, gsdir, and gsdatadir in all 3 install
# rules, just in case bindir or scriptdir is a subdirectory of any of these.

install-exec: STDDIRS $(GS_XE)
	-mkdir -p $(DESTDIR)$(datadir)
	-mkdir -p $(DESTDIR)$(gsdir)
	-mkdir -p $(DESTDIR)$(gsdatadir)
	-mkdir -p $(DESTDIR)$(bindir)
	$(INSTALL_PROGRAM) $(GS_XE) $(DESTDIR)$(bindir)/$(GS)

install-scripts: $(PSLIBDIR)/gsnd
	-mkdir -p $(DESTDIR)$(datadir)
	-mkdir -p $(DESTDIR)$(gsdir)
	-mkdir -p $(DESTDIR)$(gsdatadir)
	-mkdir -p $(DESTDIR)$(scriptdir)
	$(SH) -c 'for f in \
gsbj gsdj gsdj500 gslj gslp gsnd \
bdftops dumphint dvipdf eps2eps font2c \
pdf2dsc pdf2ps pdfopt pf2afm pfbtopfa printafm \
ps2ascii ps2epsi ps2pdf ps2pdf12 ps2pdf13 ps2pdf14 ps2pdfwr ps2ps ps2ps2 \
wftopfa fixmswrd.pl lprsetup.sh pj-gs.sh pv.sh sysvlp.sh unix-lpr.sh ;\
	do if ( test -f $(PSLIBDIR)/$$f ); then \
	  (cat $(PSLIBDIR)/$$f | sed -e "s/GS_EXECUTABLE=[^ \t]*/GS_EXECUTABLE=$(GS)/" > $(PSOBJDIR)/$$f); \
	  $(INSTALL_PROGRAM) $(PSOBJDIR)/$$f $(DESTDIR)$(scriptdir)/$$f; \
	fi;\
	done'

PSRESDIR=$(PSLIBDIR)/../Resource
PSDOCDIR=$(PSLIBDIR)/../doc
PSEXDIR=$(PSLIBDIR)/../examples
PSMANDIR=$(PSLIBDIR)/../man

install-data: install-libdata install-resdata install-doc install-man install-examples

# There's no point in providing a complete dependency list: we include
# one file from each subdirectory just as a sanity check.

install-libdata: 
	-mkdir -p $(DESTDIR)$(datadir)
	-mkdir -p $(DESTDIR)$(gsdir)
	-mkdir -p $(DESTDIR)$(gsdatadir)
	-mkdir -p $(DESTDIR)$(gsdatadir)/lib
	$(SH) -c 'for f in \
$(EXTRA_INIT_FILES) Fontmap.GS \
ht_ccsto.ps \
acctest.ps addxchar.ps align.ps bdftops.ps \
caption.ps cid2code.ps decrypt.ps docie.ps dumphint.ps \
errpage.ps font2c.ps font2pcl.ps gslp.ps gsnup.ps image-qa.ps impath.ps \
jispaper.ps landscap.ps level1.ps lines.ps markhint.ps markpath.ps \
mkcidfm.ps opdfread.ps PDFA_def.ps PDFX_def.ps \
packfile.ps pcharstr.ps pf2afm.ps pfbtopfa.ps ppath.ps \
pphs pphs.ps \
prfont.ps printafm.ps \
ps2ai.ps ps2ascii.ps ps2epsi.ps quit.ps rollconv.ps \
showchar.ps showpage.ps stcinfo.ps stcolor.ps stocht.ps \
traceimg.ps traceop.ps type1enc.ps type1ops.ps uninfo.ps unprot.ps \
viewcmyk.ps viewgif.ps viewjpeg.ps viewmiff.ps \
viewpcx.ps viewpbm.ps viewps2a.ps \
winmaps.ps wftopfa.ps wrfont.ps zeroline.ps \
pdf2dsc.ps pdfopt.ps ;\
	do if ( test -f $(PSLIBDIR)/$$f ); then $(INSTALL_DATA) $(PSLIBDIR)/$$f $(DESTDIR)$(gsdatadir)/lib; fi;\
	done'
	$(SH) -c 'for f in $(PSLIBDIR)/gs_*.ps $(PSLIBDIR)/pdf*.ps;\
	do $(INSTALL_DATA) $$f $(DESTDIR)$(gsdatadir)/lib ;\
	done'
	$(SH) -c 'for f in $(PSLIBDIR)/*.ppd $(PSLIBDIR)/*.rpd $(PSLIBDIR)/*.upp $(PSLIBDIR)/*.xbm $(PSLIBDIR)/*.xpm;\
	do $(INSTALL_DATA) $$f $(DESTDIR)$(gsdatadir)/lib ;\
	done'

# install the default resource files
# copy in every category (directory) but CVS
RES_CATEGORIES=`ls $(PSRESDIR) | grep -v CVS` 
install-resdata: $(PSRESDIR)/Decoding/Unicode
	-mkdir -p $(DESTDIR)$(datadir)
	-mkdir -p $(DESTDIR)$(gsdir)
	-mkdir -p $(DESTDIR)$(gsdatadir)/Resource
	$(SH) -c 'for dir in $(RES_CATEGORIES); do \
	  rdir=$(DESTDIR)$(gsdatadir)/Resource/$$dir ; \
	  test -d $$rdir || mkdir -p $$rdir ; \
	  for file in $(PSRESDIR)/$$dir/*; do \
	    if test -f $$file; then $(INSTALL_DATA) $$file $$rdir ; fi \
	  done \
	done'

# install html documentation
DOC_PAGES=PUBLIC README index.html gs.css \
	   API.htm Bug-form.htm Bug-info.htm \
	   C-style.htm Changes.htm Commprod.htm Copying.htm \
	   Current.htm Deprecated.htm \
	   DLL.htm Deprecated.htm Develop.htm Devices.htm Drivers.htm \
	   Fonts.htm Helpers.htm Hershey.htm \
	   History1.htm History2.htm History3.htm History4.htm \
	   History5.htm History6.htm History7.htm History8.htm \
	   Details.htm Details8.htm \
	   Htmstyle.htm Humor.htm Issues.htm Install.htm Language.htm \
	   Lib.htm Maintain.htm Make.htm New-user.htm \
	   News.htm Projects.htm Ps-style.htm Ps2epsi.htm Ps2pdf.htm \
	   Psfiles.htm Public.htm Readme.htm Release.htm \
	   Source.htm Testing.htm Unix-lpr.htm \
	   Use.htm Xfonts.htm
install-doc: $(PSDOCDIR)/News.htm
	-mkdir -p $(DESTDIR)$(docdir)
	$(SH) -c 'for f in $(DOC_PAGES) ;\
	do if ( test -f $(PSDOCDIR)/$$f ); then $(INSTALL_DATA) $(PSDOCDIR)/$$f $(DESTDIR)$(docdir); fi;\
	done'

# install the man pages for each locale
MAN_LCDIRS=. de
MAN1_LINKS_PS2PS=eps2eps
MAN1_LINKS_PS2PDF=ps2pdf12 ps2pdf13
MAN1_LINKS_GSLP=gsbj gsdj gsdj500 gslj
install-man: $(PSMANDIR)/gs.1
	$(SH) -c 'test -d $(DESTDIR)$(mandir) || mkdir -p $(DESTDIR)$(mandir)'
	$(SH) -c 'for d in $(MAN_LCDIRS) ;\
	do man1dir=$(DESTDIR)$(mandir)/$$d/man$(man1ext) ;\
	  ( test -d $$man1dir || mkdir -p $$man1dir ) ;\
	  for f in $(PSMANDIR)/$$d/*.1 ;\
	    do $(INSTALL_DATA) $$f $$man1dir ;\
	    if ( test -f $$man1dir/ps2ps.$(man1ext) ) ;\
	      then for f in $(MAN1_LINKS_PS2PS) ;\
	        do ( cd $$man1dir; rm -f $$f.$(man1ext) ;\
			  ln -s ps2ps.$(man1ext) $$f.$(man1ext) ) ;\
	      done ;\
	    fi ;\
	    if ( test -f $$man1dir/ps2pdf.$(man1ext) ) ;\
	      then for f in $(MAN1_LINKS_PS2PDF) ;\
	        do ( cd $$man1dir; rm -f $$f.$(man1ext) ;\
			  ln -s ps2pdf.$(man1ext) $$f.$(man1ext) ) ;\
	      done ;\
	    fi ;\
	    if ( test -f $$man1dir/ps2lp.$(man1ext) ) ;\
	      then for f in $(MAN1_LINKS_GSLP) ;\
	        do ( cd $$man1dir; rm -f $$f.$(man1ext) ;\
			  ln -s gslp.$(man1ext) $$f.$(man1ext) ) ;\
	      done ;\
	    fi ;\
	  done ;\
	done'

# install the example files
install-examples:
	-mkdir -p $(DESTDIR)$(exdir)
	for f in \
alphabet.ps annots.pdf chess.ps colorcir.ps doretree.ps escher.ps \
golfer.eps grayalph.ps snowflak.ps tiger.eps vasarely.ps waterfal.ps \
ridt91.eps ;\
	do $(INSTALL_DATA) $(PSEXDIR)/$$f $(DESTDIR)$(exdir) ;\
	done

install-shared: $(GS_SHARED_OBJS)
	-mkdir -p $(DESTDIR)$(gssharedir)
	$(SH) -c 'for obj in $(GS_SHARED_OBJS); do \
	    $(INSTALL_PROGRAM) $$obj $(DESTDIR)$(gssharedir)/; done'

