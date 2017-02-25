#!/bin/sh
# This script is a helper script to change filter-audio's audio profile

if [ $# -ne 1 ]
then
    echo "Usage: $0 [highest|high|low|none]"
else
    if [ $1 = "highest" -o $1 = "high" -o $1 = "low" -o $1 = "none" ]
    then
        FIND=$(which find)
        SED=$(which sed)
        ACS=$(${FIND} /etc -name filter-audio-source.acs)
        ${SED} -i -e "s/\(audio_profile.*=\).*,/\1\ \"$1\",/g" ${ACS}
    else
        echo "Invalid profile: $1, only \"high | low | none\" are accepted."
    fi
fi
