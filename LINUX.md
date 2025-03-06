# Linux

Rose was mainly developed in Windows using Visual Studio Community 2019 and 2022.
Here are some information on how to build on Linux using cmake, vscode and common developer tools.

## Configuration

The following code represents a configuration to launch with GDB, this can be added in launch.json in vscode.

```json
// !launch.json
{
	"name": "Debug with GDB",
	"type": "cppdbg",
	"request": "launch",
	"program": "${workspaceFolder}/../../build/${workspaceFolderBasename}/bin/rose",
	"args": [],
	"stopAtEntry": false,
	"cwd": "${workspaceFolder}",
	"environment": [],
	"externalConsole": false,
	"MIMode": "gdb",
	"setupCommands": [
		{
			"description": "Enable pretty-printing for gdb",
			"text": "-enable-pretty-printing",
			"ignoreFailures": true
		}
	],
	"miDebuggerPath": "/usr/bin/gdb",
	"preLaunchTask": "make",
	"serverLaunchTimeout": 20000,
	"launchCompleteCommand": "exec-run"
}
```

## Tasks

The following code represents two tasks for vscode, one for generating the build files and one for building the project, 
this can be added in tasks.json in vscode.

```json
// !tasks.json
{
	"label": "build",
	"type": "shell",
	"command": "cmake",
	"args": [
		"-DCMAKE_BUILD_TYPE=Debug",
		"-S", "${workspaceFolder}",
		"-B", "${workspaceFolder}/../../build/${workspaceFolderBasename}"
	],
	"group": {
		"kind": "build",
		"isDefault": true
	},
	"problemMatcher": ["$gcc"],
	"detail": "Generated task to build project"
},
{
	"label": "make",
	"type": "shell",
	"command": "make",
	"args": [
		"-C", "${workspaceFolder}/../../build/${workspaceFolderBasename}"
	],
	"group": "build",
	"dependsOn": "build",
	"problemMatcher": ["$gcc"],
	"detail": "Run make to compile the project"
}
```