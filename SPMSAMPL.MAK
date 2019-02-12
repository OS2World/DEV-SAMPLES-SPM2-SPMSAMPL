#--------------------------------------------------------------------------
# spm api sample makefile
#--------------------------------------------------------------------------

LFLAGS= /nod /map /co /al:16
CFLAGS= Cl -c -Zp -G2s -Alfu -W3 -Zi -Od -Fc

LDLL1 = spmapi doscalls llibce os2

spmsampl.exe: spmsampl.obj spmsample.def spmsampl.mak
     link spmsampl.obj,,,$(LDLL1)$(LFLAGS), spmsampl

spmsampl.obj:    spmsampl.c $(HEADERS) spmsampl.mak
      $(CFLAGS) spmsampl.c

