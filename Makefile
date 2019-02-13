CXX = g++
CXXFLAGS = -Wall -pedantic
SRCDIR = src
OBJDIR = obj

EXECUTABLE_NAME = mtte
CXXSRC = $(wildcard $(SRCDIR)/*.cpp)
CXXOBJ = $(patsubst %.cpp,%.o,$(subst $(SRCDIR)/,$(OBJDIR)/,$(CXXSRC)))


# $@: target of a rule
# $^: sources of a rule
# $<: first source of a rule

mtte: create_dir build
	@echo "Make successful"

build: $(CXXOBJ)
	$(CXX) -o $(EXECUTABLE_NAME) $(CXXFLAGS) $^

obj/main.o: src/main.cpp src/terminal.h src/editor.h src/keyboard.h
	$(CXX) -c -o $@ $(CXXFLAGS) $<

obj/terminal.o: src/terminal.cpp src/terminal.h src/error.h
	$(CXX) -c -o $@ $(CXXFLAGS) $<

obj/editor.o: src/editor.cpp src/editor.h src/cursor.h src/render.h src/keyboard.h src/row.h
	$(CXX) -c -o $@ $(CXXFLAGS) $<

obj/error.o: src/error.cpp src/error.h
	$(CXX) -c -o $@ $(CXXFLAGS) $<	

obj/cursor.o: src/cursor.cpp src/cursor.h
	$(CXX) -c -o $@ $(CXXFLAGS) $<

obj/render.o: src/render.cpp src/render.h src/row.h src/config.h src/error.h
	$(CXX) -c -o $@ $(CXXFLAGS) $<

obj/keyboard.o: src/keyboard.cpp src/keyboard.h src/cursor.h src/render.h src/error.h
	$(CXX) -c -o $@ $(CXXFLAGS) $<

obj/row.o: src/row.cpp src/row.h
	$(CXX) -c -o $@ $(CXXFLAGS) $<

# Automatically generates dependencies from file list (not complete)
# $(OBJDIR)/%.o: $(SRCDIR)/%.cpp
# 	$(CXX) -c -o $@ $(CXXFLAGS) $<

.PHONY: clean
clean:
	rm -f $(CXXOBJ)

create_dir:
	@echo "Making object directory"
	@echo "Object files:" $(CXXOBJ)
	@mkdir -p $(OBJDIR)
