INCLUDES = \
	-I../../../../../tufsbox/cdkroot/usr/include \
	-I../../../../../tufsbox/cdkroot/usr/include/glib-2.0 \
	-I../../../../../tufsbox/cdkroot/usr/include/gstreamer-0.10 \
	-I../../../../../tufsbox/cdkroot/usr/include/libxml2 \
	-I../../../../../tufsbox/cdkroot/usr/lib/glib-2.0/include

bin_PROGRAMS = gstplayer
gstplayer_SOURCES = gstplayer.c gst-backend.c
gstplayer_LDADD = -lgstreamer-0.10 -lgstbase-0.10 -lgstinterfaces-0.10
