all: compile run

clean:
	echo "\n\033[1;91m█ Cleaning build directory...\033[0m\n" &&\
	rm -rf build/*

compile:
	@mkdir -p build &&\
	cd build &&\
	echo "\n\033[1;33m█ Generating makefiles...\033[0m\n" &&\
	cmake .. &&\
	echo "\n\033[1;93m█ Compiling...\033[0m\n" &&\
	make

run:
	@echo "\n\033[1;32m█ Running...\033[0m\n\n" &&\
	./build/WheelController/WheelController

runwheel:
	@echo "\n\033[1;32m█ Running...\033[0m\n\n" &&\
	./build/WheelController/WheelController
