CXX = g++
#CXXFLAGS = -Wall -Werror -Wextra -pedantic -std=c++17 -g -fsanitize=address -Wno-missing-field-initializers -Wno-unused-function -I.
CXXFLAGS = -Wall -Werror -Wextra -pedantic -std=c++17 -g -fsanitize=address -Wno-missing-field-initializers -Wno-unused-function -Wno-error=unused-variable -Wno-format-truncation  -I. -g
LDFLAGS =  -fsanitize=address

SRC_CC = gpbdecoder.cc
SRC_CPP = gbp_pkt.cpp gbp_tiles.cpp gbp_bmp.cpp
OBJ = $(SRC_CC:.cc=.o) $(SRC_CPP:.cpp=.o)
EXEC = gpbdecoder

ODIR=obj

all: $(EXEC)

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

test: $(EXEC)
	@echo "Test..."
	@cat ./test/test.txt | ./$(EXEC) -p "#ffffff#ffad63#833100#000000" -o ./test/test.bmp
	./$(EXEC) -p "#dbf4b4#abc396#7b9278#4c625a#FFFFFF00" -i ./test/2020-08-10_Pokemon_trading_card_compressiontest.txt

testdisplay: $(EXEC)
	@echo "Test..."
	./$(EXEC) -i ./test/2020-08-10_Pokemon_trading_card_compressiontest.txt -d
	./$(EXEC) --help

debug: $(EXEC)
	gdb ./$(EXEC)

other:
	./$(EXEC)

flagsSRC:
	@echo $(SRC_CC) $(SRC_CPP)
flagsOBJ:
	@echo $(OBJ)