CXX = g++
CXXFLAGS = -Wall

compile: reader_writer

reader_writer: reader_writer.cpp cond.c
	$(CXX) $(CXXFLAGS) -o reader_writer reader_writer.cpp

run: reader_writer
	./reader_writer

clean:
	rm -f reader_writer
