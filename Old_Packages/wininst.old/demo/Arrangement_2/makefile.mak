# Created by the script create_makefile
# This is the makefile for compiling a CGAL application.

#---------------------------------------------------------------------#
#                    include platform specific settings
#---------------------------------------------------------------------#
# Choose the right include file from the <cgalroot>/make directory.

# CGAL_MAKEFILE = ENTER_YOUR_INCLUDE_MAKEFILE_HERE
!include $(CGAL_MAKEFILE)

#---------------------------------------------------------------------#
#                    compiler flags
#---------------------------------------------------------------------#

CXXFLAGS = \
           $(CGAL_CXXFLAGS) \
           $(LONG_NAME_PROBLEM_CXXFLAGS) \
           $(DEBUG_OPT)

#---------------------------------------------------------------------#
#                    linker flags
#---------------------------------------------------------------------#

LIBPATH = \
          $(CGAL_WINDOW_LIBPATH)

LDFLAGS = \
          $(LONG_NAME_PROBLEM_LDFLAGS) \
          $(CGAL_WINDOW_LDFLAGS)

#---------------------------------------------------------------------#
#                    target entries
#---------------------------------------------------------------------#

all:            \
                Circle_arr_from_file$(EXE_EXT) \
                Segment_arr_from_file$(EXE_EXT) \
                Segment_arr_from_mouse$(EXE_EXT) 

Circle_arr_from_file$(EXE_EXT): Circle_arr_from_file$(OBJ_EXT)
	$(CGAL_CXX) $(LIBPATH) $(EXE_OPT)Circle_arr_from_file Circle_arr_from_file$(OBJ_EXT) $(LDFLAGS)

Segment_arr_from_file$(EXE_EXT): Segment_arr_from_file$(OBJ_EXT)
	$(CGAL_CXX) $(LIBPATH) $(EXE_OPT)Segment_arr_from_file Segment_arr_from_file$(OBJ_EXT) $(LDFLAGS)

Segment_arr_from_mouse$(EXE_EXT): Segment_arr_from_mouse$(OBJ_EXT)
	$(CGAL_CXX) $(LIBPATH) $(EXE_OPT)Segment_arr_from_mouse Segment_arr_from_mouse$(OBJ_EXT) $(LDFLAGS)

clean: \
                   Circle_arr_from_file.clean \
                   Segment_arr_from_file.clean \
                   Segment_arr_from_mouse.clean 

#---------------------------------------------------------------------#
#                    suffix rules
#---------------------------------------------------------------------#

.C$(OBJ_EXT):
	$(CGAL_CXX) $(CXXFLAGS) $(OBJ_OPT) $<

