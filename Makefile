
#----------------
# The C and C++ compilers to use.
#----------------
CC = gcc
CCC = g++
CXX = $(CCC)


#----------------
#set directories for project
#----------------
INC_DIR = src
DOC_DIR = src
SRC_DIR = src
LIB_DIR = src/lib/mingw32
RES_DIR = src/resources


#----------------
# set compiler-options
#----------------
ifeq ($(DEBUG), YES)
	CFLAGS = -Wno-deprecated -mthreads -I $(INC_DIR) -D_DEBUG_ -g
	OBJ_DIR = debug
	EXE_DIR = bin
	LIB_ADD = _d
else
	#CFLAGS =-O1 -Wno-deprecated -mthreads -I $(INC_DIR) -DAUTOITSC
	#CFLAGS =-O -Wall -Wno-deprecated -mthreads -I $(INC_DIR)
	#CFLAGS =-O -Wno-deprecated -mthreads -I $(INC_DIR)
	CFLAGS =-O1 -Wno-deprecated -mthreads -I $(INC_DIR)
	OBJ_DIR = release
	EXE_DIR = bin
	LIB_ADD = 
endif


#----------------
#set file groups
#----------------

OBJECTS =		$(OBJ_DIR)/application.o	\
			$(OBJ_DIR)/astring_datatype.o	\
			$(OBJ_DIR)/AutoIt.o		\
			$(OBJ_DIR)/cmdline.o		\
			$(OBJ_DIR)/globaldata.o		\
			$(OBJ_DIR)/mt19937ar-cok.o	\
			$(OBJ_DIR)/os_version.o		\
			$(OBJ_DIR)/script.o		\
			$(OBJ_DIR)/setforegroundwinex.o	\
			$(OBJ_DIR)/script_lexer.o	\
			$(OBJ_DIR)/script_parser.o	\
			$(OBJ_DIR)/script_parser_exp.o	\
			$(OBJ_DIR)/script_process.o	\
			$(OBJ_DIR)/script_win.o		\
			$(OBJ_DIR)/script_file.o	\
			$(OBJ_DIR)/script_gui.o	\
			$(OBJ_DIR)/script_misc.o	\
			$(OBJ_DIR)/script_math.o	\
			$(OBJ_DIR)/script_registry.o	\
			$(OBJ_DIR)/script_string.o	\
			$(OBJ_DIR)/scriptfile.o		\
			$(OBJ_DIR)/utility.o		\
			$(OBJ_DIR)/token_datatype.o	\
			$(OBJ_DIR)/variant_datatype.o	\
			$(OBJ_DIR)/stack_int_datatype.o	\
			$(OBJ_DIR)/stack_variant_datatype.o	\
			$(OBJ_DIR)/stack_statement_datatype.o	\
			$(OBJ_DIR)/vector_token_datatype.o	\
			$(OBJ_DIR)/vector_variant_datatype.o	\
			$(OBJ_DIR)/variabletable.o	\
			$(OBJ_DIR)/variable_list.o	\
			$(OBJ_DIR)/stack_variable_list.o	\
			$(OBJ_DIR)/userfunction_list.o	\
			$(OBJ_DIR)/inputbox.o	\
			$(OBJ_DIR)/sendkeys.o	\
			$(OBJ_DIR)/guibox.o	\
			$(OBJ_DIR)/shared_memory.o	\
			$(OBJ_DIR)/AutoIt.res.o

LIBS	=	-lwinmm -lversion -lwsock32 -lole32 -loleaut32 -luuid -lcomctl32 -lmpr
			

RESOURCES = $(RES_DIR)/AutoIt.rc

TARGET =	AutoIt3


#----------------
#LinkerFlags
#----------------
LDFLAGS =  -mwindows -L$(LIB_DIR) 


#----------------
#*** Makerules ***
#----------------
.SUFFIXES: .o .cpp .c .rc
# allgemein Regeln

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $(OBJ_DIR)/$*.o

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.cpp
	$(CCC) $(CFLAGS) -c $< -o $(OBJ_DIR)/$*.o

$(OBJ_DIR)/%.res.o : $(RES_DIR)/%.rc
	windres --include-dir $(RES_DIR) -i $< -o $@


#----------------
#*** Targets  ***
#----------------

all: AutoIt

AutoIt: $(EXE_DIR)/$(TARGET).exe

$(EXE_DIR)/$(TARGET).exe : $(OBJECTS)
	$(CXX) $(LDFLAGS) $(CFLAGS) $(OBJECTS) -o $@ $(LIBS) -m486
	strip $@
	$(EXE_DIR)/upx.exe --best --compress-icons=0 $(EXE_DIR)/$(TARGET).exe


clean:
	rm -f $(OBJ_DIR)/*.o
	rm -f $(EXE_DIR)/$(TARGET).exe

