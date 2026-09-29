.PHONY: all clean

CFLAGS := -std=c11 -Wall -Wextra -Werror \
	-I../ios/ios_pad/source -I../libbloopair/include
CXXFLAGS := -std=c++20 -Wall -Wextra -Werror \
	-I../libbloopair/include

all: test_nintendont_pairing test_pairing_export test_nintendont_sync
	./test_nintendont_pairing
	./test_pairing_export
	./test_nintendont_sync

test_nintendont_pairing: test_nintendont_pairing.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

test_pairing_export: test_pairing_export.c \
	../ios/ios_pad/source/pairing_export.c
	$(CC) $(CFLAGS) $^ -o $@

test_nintendont_sync: test_nintendont_sync.cpp
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	$(RM) test_nintendont_pairing test_pairing_export test_nintendont_sync
