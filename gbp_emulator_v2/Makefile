CXX = g++
CXXFLAGS = -Wall -Werror -Wextra -pedantic -std=c++17 -g -fsanitize=address -Wno-missing-field-initializers -Wno-unused-function -I.
LDFLAGS =  -fsanitize=address

SRC_CC = test/gpb_test.cc
SRC_CPP = gbp_serial_io.cpp gbp_pkt.cpp
OBJ = $(SRC_CC:.cc=.o) $(SRC_CPP:.cpp=.o)
EXEC = gpb_test

ODIR=obj

all: $(EXEC) run clean

%.o: %.cc
	$(CXX) $ -c -o $@ $< $(CXXFLAGS)

%.o: %.cpp
	$(CXX) $ -c -o $@ $< $(CXXFLAGS)

$(EXEC): $(OBJ)
	@echo "Building..."
	$(CXX) $(LDFLAGS) -o $@ $(OBJ) $(LBLIBS)

clean:
	@echo "Cleaning..."
	rm -rf $(OBJ) $(EXEC)

run:
	@echo "Running..."
	./$(EXEC)

flagsSRC:
	@echo $(SRC_CC) $(SRC_CPP)
flagsOBJ:
	@echo $(OBJ)