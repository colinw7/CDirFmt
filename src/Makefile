CPP_CC = g++

RM = rm

CDEBUG = -g
LDEBUG = -g

INC_DIR = ../include
OBJ_DIR = ../obj
LIB_DIR = ../lib
BIN_DIR = ../bin

all: $(BIN_DIR)/CDirFmt

clean:
	$(RM) -f $(OBJ_DIR)/*.o
	$(RM) -f $(BIN_DIR)/CDirFmt

CPP_SRC = \
CDirFmt.cpp \

CPP_OBJS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(CPP_SRC))

CPPFLAGS = \
-std=c++11 \
-I$(INC_DIR) \
-I../../CArgs/include \
-I../../CFile/include \
-I../../CFileUtil/include \
-I../../COS/include \
-I../../CRegExp/include \
-I../../CStrUtil/include \
-I../../CUtil/include \
-I.

LFLAGS = \
$(LEBUG) \
-L$(LIB_DIR) \
-L../../CArgs/lib \
-L../../CFile/lib \
-L../../CFileUtil/lib \
-L../../COS/lib \
-L../../CRegExp/lib \
-L../../CStrUtil/lib \
-lCArgs \
-lCFileUtil \
-lCFile \
-lCOS \
-lCRegExp \
-lCStrUtil \
-ltre

.SUFFIXES: .cpp

$(CPP_OBJS): $(OBJ_DIR)/%.o: %.cpp
	$(CPP_CC) -c $< -o $(OBJ_DIR)/$*.o $(CPPFLAGS)

$(BIN_DIR)/CDirFmt: $(CPP_OBJS)
	$(CPP_CC) -o $(BIN_DIR)/CDirFmt $(CPP_OBJS) $(LFLAGS)
