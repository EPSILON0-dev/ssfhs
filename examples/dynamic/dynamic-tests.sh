#!/bin/sh
# SimpleHTTP dynamic tests script

# Check if the form was submitted
METHOD=`echo "$REQUEST_STR" | head -1 | cut -d' ' -f1`
if [ "$METHOD" != "POST" ]; then
    exit 0
fi

# Extract the input message from the request body
INPUT_MSG=`echo "$REQUEST_STR" | grep bc-input= | cut -d'=' -f2-`

# URL decode the input message
INPUT_MSG_DECODED=`printf '%b' "$(echo "$INPUT_MSG" | sed 's/+/ /g;s/%/\\\x/g')"`

# Send it to bc
printf "<b>bc</b> returned: "
echo "$INPUT_MSG_DECODED" | bc -l 2>&1