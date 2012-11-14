if exist ..\include\wx\msw\setup.h (
    echo include\wx\msw\setup.h already exists
) else (
    copy /y ..\include\wx\msw\setup0.h ..\include\wx\msw\setup.h
)

if exist ..\lib\cw7msw (
    echo lib\cw7msw already exists
) else (
    mkdir ..\lib\cw7msw
)

if exist ..\lib\cw7msw\include (
    echo lib\cw7msw\include already exists
) else (
    mkdir ..\lib\cw7msw\include
)

if exist ..\lib\cw7msw\include\wx (
    echo lib\cw7msw\include\wx already exists
) else (
    mkdir ..\lib\cw7msw\include\wx
)

if exist ..\lib\cw7msw\include\wx\setup.h (
    echo lib\cw7msw\include\wx\setup.h already exists
) else (
    copy /y ..\include\wx\msw\setup.h ..\lib\cw7msw\include\wx\setup.h
)

rem pause

