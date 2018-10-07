#n

# sed script to simulate the BSD cat -s command,
# squeezing excess blank lines from the input.

# write non-empty lines

/./{
    p
    d
}

# Write a single empty line.

/^$/p

# Get next line, discard the <newline> character
# inserted by the 'N' command, then check for more
# adjacent empty lines.

:Empty
/^$/{
    N
    s/.//
    b Empty
}

# Write the non-empty line read by 'N' above, and
# restart the search for the next set of empty lines.

p
