CXX = g++
#CXXFLAGS = -Wall -Werror -Wextra -pedantic -std=c++17 -g -fsanitize=address -Wno-missing-field-initializers -Wno-unused-function -I.
CXXFLAGS = -Wall -Werror -Wextra -pedantic -std=c++17 -g -fsanitize=address -Wno-missing-field-initializers -Wno-unused-function -Wno-error=unused-variable -I.
LDFLAGS =  -fsanitize=address

SRC_CC = gpbdecoder.cc
SRC_CPP = gbp_pkt.cpp
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
	@cat ./test/2020-08-10_Pokemon_trading_card_compressiontest.txt | ./$(EXEC)
	@cat ./test/2020-08-10_Pokemon_trading_card_compressiontest.txt | ./$(EXEC) >> output.txt

other:
	./$(EXEC)

flagsSRC:
	@echo $(SRC_CC) $(SRC_CPP)
flagsOBJ:
	@echo $(OBJ)