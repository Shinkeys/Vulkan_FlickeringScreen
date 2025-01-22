	echo Compiling shader...
..\bin\glslangValidator  -V -g ..\shaders\main.vert -o ..\shaders\main_vert.spv
..\bin\glslangValidator  -V -g ..\shaders\main.frag -o ..\shaders\main_frag.spv

cmd /k