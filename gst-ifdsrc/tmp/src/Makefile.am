# Note: plugindir is set in configure

##############################################################################
# TODO: change libgstifdsrc.la to something else, e.g. libmysomething.la     #
##############################################################################
plugin_LTLIBRARIES = libgstifdsrc.la

##############################################################################
# TODO: for the next set of variables, name the prefix if you named the .la, #
#  e.g. libmysomething.la => libmysomething_la_SOURCES                       #
#                            libmysomething_la_CFLAGS                        #
#                            libmysomething_la_LIBADD                        #
#                            libmysomething_la_LDFLAGS                       #
##############################################################################

# sources used to compile this plug-in
libgstifdsrc_la_SOURCES = gstifdsrc.c gstifdsrc.h

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstifdsrc_la_CFLAGS = $(GST_CFLAGS)
libgstifdsrc_la_LIBADD = $(GST_LIBS)
libgstifdsrc_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstifdsrc_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstifdsrc.h
