import os
import sys

exePath = "HLSLtoGLSL\\win64\\HLSL2GLSLConverter.exe"
D3DDirectory = "D3D11"
GLDirectory = "OpenGL"
CurrentPath = os.getcwd()


def Convert():
    print("Function call")
    # assign directory

    # iterate over files in
    # that directory
    for filename in os.listdir(D3DDirectory):
        f = os.path.join(D3DDirectory, filename)
        # checking if it is a file
        shaderType = getType(f)
        fullfilePath = os.path.abspath(f)

        fileNameWithoutExtension = os.path.splitext(filename)[0]

        command = f"-i {CurrentPath}\\{f} -t {shaderType} -e {getEntryByType(shaderType)} -o {CurrentPath}\\{GLDirectory}\\{fileNameWithoutExtension}.glsl "
        print(f"Command to be run: {command}")

        os.system(f"{CurrentPath}\\{exePath} {command}")
        #print(f"{CurrentPath}\\{exePath} {command}")


def getEntryByType(inType):
    if ("ps" in inType):
        return "PSEntry"
    if ("vs" in inType):
        return "VSEntry"


def getType(filePath):
    if ("_PS" in filePath):
        return "ps"
    if ("_VS" in filePath):
        return "vs"


def show_exception_and_exit(exc_type, exc_value, tb):
    import traceback
    traceback.print_exception(exc_type, exc_value, tb)
    input("Press enter to exit.")
    sys.exit(-1)


sys.excepthook = show_exception_and_exit

Convert()

input("Press enter to exit.")
