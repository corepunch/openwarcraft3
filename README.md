# openwarcraft3
Open source re-implementation of Warcraft III

Currently support War3.mpq from Warcraft III v1.0 or similar

<img width="1025" alt="Screenshot 2023-05-25 at 09 57 57" src="https://github.com/corepunch/openwarcraft3/assets/83646194/643c7aa7-2b91-469c-857e-0f6910c939af">

<img width="1026" alt="Screenshot 2023-05-25 at 09 58 15" src="https://github.com/corepunch/openwarcraft3/assets/83646194/a79e447d-e42c-4468-b4ca-3d212efe346a">

## Cloning

    git clone --recurse-submodules git@github.com:corepunch/openwarcraft3.git

## Create project

    tools\bin\windows\premake5.exe vs2022

Or mac

    tools/bin/darwin/premake5 xcode4

or

    tools/bin/darwin/premake5 --cc=clang gmake

The project files will be created in the build folder.
