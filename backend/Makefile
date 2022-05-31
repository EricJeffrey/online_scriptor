#----------------------------------------
#     Prepare flags from make generator
#----------------------------------------

include ./build/conanbuildinfo.mak

CFLAGS              += $(CONAN_CFLAGS)
CXXFLAGS            += $(CONAN_CXXFLAGS)
CXXFLAGS 			+= -std=c++17
CPPFLAGS            += $(addprefix -I, $(CONAN_INCLUDE_DIRS))
CPPFLAGS            += $(addprefix -D, $(CONAN_DEFINES))
LDFLAGS             += $(addprefix -L, $(CONAN_LIB_DIRS))
LDLIBS              += $(addprefix -l, $(CONAN_LIBS))
LDLIBS				+= -lpthread
EXELINKFLAGS        += $(CONAN_EXELINKFLAGS)

#----------------------------------------
#     Make variables for a sample App
#----------------------------------------

SRCS          = task_manager.cpp
OBJS          = task_manager.o
EXE_FILENAME  = task_manager
OUTPUT_DIR    = build

#----------------------------------------
#     Make Rules
#----------------------------------------

.PHONY                  :   exe
.PHONY                  :   clean
exe                     :   $(EXE_FILENAME)

$(EXE_FILENAME)         :   $(OBJS)
	g++ $(OBJS) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS) -o $(OUTPUT_DIR)/$(EXE_FILENAME)

*.o                     :   $(SRCS)
	g++ -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.test.out				: %.cpp
	g++ -g -D TESTING $< $(CXXFLAGS) $(LDFLAGS) $(LDLIBS) -o testing/$@

clean:
	rm -rf $(OUTPUT_DIR)/$(EXE_FILENAME) *.o