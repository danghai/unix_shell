#n

# script to simulate the diff -e command, whereby
# WinXs diff ouput is converted to a sed script.
# When applied to file1, this script will produce
# file2.

# delete lines referring to file1

:begin
/^----/d
/^< /d

# pass through blank lines

/^$/{
    p
    d
}

# convert sequences from file2

/^> /{
    s/^> //
    x
:loop
    n
    s/^> //
    t repeat
    x
    p
    x
    b begin
:repeat
    x
    s/$/\\/
    p
    b loop
}

# convert command lines

/a/{
    s/a.*$/a\\/
    s/^0a/1i/
    p
    d
}

/c/{
    s/c.*$/c\\/p
    d
}

/d/{
    s/d.*$/d/p
    d
}
