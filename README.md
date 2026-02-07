# Rose

The most common phrase I hear when describing the purpose of a software is "World Domination", shall we then...

## Quick Information

Yet another graphics engine developed by a single student with too much free time in his hands (kinda). 
Drawing inspiration from differential geometry for normal calculation, triangulation and animation of objects, I am a math student after all.

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

![Demo](https://i.imgur.com/wUXscjb.png)

# Authors

Dimitris Bokis,
undergraduate student at Aristotle University of Thessaloniki
