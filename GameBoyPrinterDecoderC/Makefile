CXX = g++
#CXXFLAGS = -Wall -Werror -Wextra -pedantic -std=c++17 -g -fsanitize=address -Wno-missing-field-initializers -Wno-unused-function -I.
CXXFLAGS = -Wall -Werror -Wextra -pedantic -std=c++17 -g -fsanitize=address -Wno-missing-field-initializers -Wno-unused-function -Wno-error=unused-variable -Wno-format-truncation  -I. -g
LDFLAGS =  -fsanitize=address

SRC_CC = gpbdecoder.cc
SRC_CPP = gbp_pkt.cpp gbp_tiles.cpp gbp_bmp.cpp
OBJ = $(SRC_CC:.cc=.o) $(SRC_CPP:.cpp=.o)
EXEC = gpbdecoder

ODIR=obj

all: $(EXEC) run

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
	@cat ./test/test.txt | ./$(EXEC)

debug:
	gdb ./$(EXEC)

other:
	./$(EXEC)

flagsSRC:
	@echo $(SRC_CC) $(SRC_CPP)
flagsOBJ:
	@echo $(OBJ)