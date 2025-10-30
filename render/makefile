renderByInstance:
	g++ -std=c++17 -O2 -Wall -Wextra modelClass.cpp sceneBuilderClass.cpp main.cpp -o renderByInstance \
	-lglfw -lGLEW -lGL -lassimp

# Run with arguments, e.g.:
# make run ARGS="assets/bunny.obj 100"
run: renderByInstance
	./renderByInstance $(ARGS)

clean:
	rm -f renderByInstance