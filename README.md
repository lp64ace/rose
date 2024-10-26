# Rose

Cross-platform library that enables easy customizability of the application during both runtime and develop-time.

## Quick Information

Rose aims to be a software development tool that allows easy development and management of an application both in runtime and develop-time. 
By employing the intergrated c99 interperter we can quickly view/edit data in runtime and store them in the application's database file for later usage.

## How to Build

### Prerequisites

Before proceeding, ensure that the following prerequisites are met:

- **Git:** Make sure Git is installed on your system.
- **CMake:** Ensure CMake is installed and accessible from your command prompt.

## Linux

- **Χ11** On Linux systems, X11 is required in order to have GUI.
- **GLEW** On Linux systems, GLEW is required in order to have GUI.
- **Mesa** On Linux systems, Mesa is required in order to have GUI.

1. Install dependencies:

	```bash
	sudo apt-get update
	sudo apt-get install -y libx11-dev
	sudo apt-get install -y libglew-dev
	sudo apt-get install -y libgl1-mesa-dev libglu1-mesa-dev
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

# Authors

Dimitris Bokis,
undergraduate student at Aristotle University of Thessaloniki
