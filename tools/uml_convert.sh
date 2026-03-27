#!/bin/bash

if [ -z "$1" ]; then
	echo "Usage: ./uml_convert.sh [filename]"
	exit 1
fi

FILE=$1

echo "Converting $FILE to UML notation..."

# Using a more robust approach that avoids complex split-regex
perl -i -pe '
if (s/virtual\s+([\w:<>\s*&]+)\s+(\w+)\((.*)\)\s*(?:const)?\s*=\s*0;/@/ ) {
    $ret = $1;
    $name = $2;
    $params_str = $3;
    
    my @flipped = ();
    
    # This regex matches a Type (including <...>) followed by a Name
    # It looks for "Anything followed by a word" before a comma or end of string
    while ($params_str =~ m/([^,]+(?:<[^<>]+>)?)\s+(\w+)(?:,|$)/g) {
        my $type = $1;
        my $pname = $2;
        $type =~ s/^\s+|\s+$//g; # Trim whitespace
        push(@flipped, "$pname: $type");
    }
    
    $_ = "    +$name(" . join(", ", @flipped) . "): $ret\n";
}' "$FILE"

echo "Done! Results in $FILE"
