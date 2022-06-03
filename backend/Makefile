#----------------------------------------
#     Prepare flags from make generator
#----------------------------------------

include ./build/conanbuildinfo.mak

CFLAGS              += $(CONAN_CFLAGS)
CXXFLAGS            += $(CONAN_CXXFLAGS)
CXXFLAGS 			+= -std=c++17 -fstack-protector -pipe -Wall -Werror -Wshadow -Wno-unused-variable -Wfloat-equal -Wsign-compare -Wconversion
CPPFLAGS            += $(addprefix -I, $(CONAN_INCLUDE_DIRS))
CPPFLAGS            += $(addprefix -D, $(CONAN_DEFINES))
LDFLAGS             += $(addprefix -L, $(CONAN_LIB_DIRS))
LDLIBS              += $(addprefix -l, $(CONAN_LIBS))
LDLIBS				+= -lpthread
EXELINKFLAGS        += $(CONAN_EXELINKFLAGS)

#----------------------------------------
#     Make variables for a sample App
#----------------------------------------

SRCS          = *.cpp
HDRS		  = *.hpp
OBJS          = cmd_mgr.o cmd_msg.o db_guard.o io_data_msg.o io_mgr.o script_helper.o task.o task_db_helper.o
OUTPUT_DIR    = build

#----------------------------------------
#     Make Rules
#----------------------------------------

.PHONY                  :   main
.PHONY                  :   clean

main			        :
	g++-9 -D TESTING $(SRCS) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS) -o $(OUTPUT_DIR)/main

*.o                     :   *.cpp
	g++-9 -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

clean:
	rm -rf $(OUTPUT_DIR)/main* *.o