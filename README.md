# Rose

Cross-platform library that enables easy customizability of the application during both runtime and develop-time.
It's the fact that there are limits that make life interesting, wouldn't it be boring to be able to do ANYTHING, 
it would definately be scary!

## Quick Information

Rose aims to be a software development tool that allows easy development and management of an application both in runtime and develop-time. 
By employing the intergrated c99 interperter we can quickly view/edit data in runtime and store them in the application's database file for later usage.

Yet another library that feels like fighting with it rather than your project! You're welcome!

## How to Build

### Prerequisites

Before proceeding, ensure that the following prerequisites are met:

- **Git:** Make sure Git is installed on your system.
- **CMake:** Ensure CMake is installed and accessible from your command prompt.

## (Linux) Installation Instructions

Before building the project, ensure that the following development libraries are installed on your Linux system.
This project requires **X11**, **GLEW**, and **MESA** development tools.

1. Install the required dependencies:

	```bash
	sudo apt-get update
	sudo apt-get install -y meson pkg-config ragel gtk-doc-tools
	sudo apt-get install -y libx11-dev libxi-dev
	sudo apt-get install -y libglew-dev libgl1-mesa-dev libglu1-mesa-dev
	sudo apt-get install -y libfreetype6-dev libglib2.0-dev libcairo2-dev
	```

## Building Rose

1. Clone the Rose repository:

	```bash
	git clone https://github.com/lp64ace/rose.git
	```

2. Navigate to a folder where you want the build files to be generated:

	```bash
	mkdir build
	cd build
	cmake ../rose
	```
3. Compile the project, and enjoy!

	```bash
	cmake --build .
	```

# Showcase

Here is a little preview of the application running in its current state, this is simply a 
showcase demo.

![Demo](https://i.imgur.com/HG7tux9.gif)

# Authors

Dimitris Bokis,
undergraduate student at Aristotle University of Thessaloniki
