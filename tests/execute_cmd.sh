#!/bin/bash

SCRIPT_PATH=$(dirname $(realpath -s $0))

echo "$0 $@" > ${SCRIPT_PATH}/_output/last_execute_cmd

if [[ -n ${RETURN_CODE:-} ]]; then
    exit ${RETURN_CODE}
else
    exit 0
fi