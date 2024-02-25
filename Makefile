OBJECTS=waadr.o
BINARIES=waadr

RGB_LIB_DISTRIBUTION=matrix
RGB_INCDIR=$(RGB_LIB_DISTRIBUTION)/include
RGB_LIBDIR=$(RGB_LIB_DISTRIBUTION)/lib
RGB_LIBRARY_NAME=rgbmatrix
RGB_LIBRARY=$(RGB_LIBDIR)/lib$(RGB_LIBRARY_NAME).a

LDFLAGS+=-L$(RGB_LIBDIR) -l$(RGB_LIBRARY_NAME) -lrt -lm -lpthread

waadr : $(OBJECTS) $(RGB_LIBRARY)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(RGB_LIBRARY):
	$(MAKE) -C $(RGB_LIBDIR)

waadr.o : waadr.cpp

%.o : %.cpp
	$(CXX) -I$(RGB_INCDIR) -c -o $@ $<


