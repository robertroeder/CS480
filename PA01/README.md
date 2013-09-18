## Project 1
### Purpose
The purpose of this assignment was to:
- Add rotation to an orbiting cube
- Remove hard coded shaders and implement a shader loader

### Problems Encountered
- GLM Rotate function requires a float in degrees, not a double or int
- Returning a pointer to the shader from the load function needed to preserve the memory that was allocated inside the function

### Instructions
- Enter the "build" directory and type make
- Enter the "bin" directory and run the Matrix executable
